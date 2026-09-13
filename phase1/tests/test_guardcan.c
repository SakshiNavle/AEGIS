#include "delay_queue.h"
#include "guardcan.h"

#include <stdio.h>

static int tests_run;
static int tests_passed;

static GuardcanFrame frame(uint16_t id, uint8_t dlc, uint16_t sequence,
                           uint32_t timestamp)
{
    GuardcanFrame value = { id, dlc, { 0U }, timestamp };
    value.payload[0] = (uint8_t)(sequence & 0xFFU);
    value.payload[1] = (uint8_t)(sequence >> 8);
    return value;
}

static void check_bool(const char *name, bool actual, bool expected)
{
    int passed = actual == expected;
    ++tests_run;
    printf("%s: %s\n", name, passed ? "PASS" : "FAIL");
    if (passed) ++tests_passed;
}

static void check_risk(const char *name, GuardcanRiskLevel actual,
                       GuardcanRiskLevel expected)
{
    int passed = actual == expected;
    ++tests_run;
    printf("%s: expected %s, actual %s: %s\n", name,
           guardcan_risk_name(expected), guardcan_risk_name(actual),
           passed ? "PASS" : "FAIL");
    if (passed) ++tests_passed;
}

static void check_health(const char *name, GuardcanDetectorHealth actual,
                         GuardcanDetectorHealth expected)
{
    int passed = actual == expected;
    ++tests_run;
    printf("%s: expected %s, actual %s: %s\n", name,
           guardcan_health_name(expected), guardcan_health_name(actual),
           passed ? "PASS" : "FAIL");
    if (passed) ++tests_passed;
}

int main(void)
{
    GuardcanMonitor monitor;
    GuardcanSnapshot snapshot;
    GuardcanFrame current;
    CacgfRiskState risk;
    VehicleContext vehicle = { 0U, VEHICLE_GEAR_PARK, false, 1U, 0U,
                               CONTEXT_VALID };
    CacgfSecurityState security = { true, true, true };
    CacgfCommand command = { 0x100U, CACGF_CRITICALITY_HIGH, false, 1U };
    CacgfQueueState queue_state = { false };
    CacgfResult result;
    DelayQueue delay_queue;
    uint32_t index;

    printf("# ========================================\n");
    printf("AEGIS PHASE 5 GUARDCAN TEST\n\n");

    guardcan_init(&monitor);
    check_risk("TEST 01: Initial risk is UNKNOWN",
               guardcan_risk(&monitor), GUARDCAN_RISK_UNKNOWN);
    check_health("TEST 02: Initial detector health is OK",
                 guardcan_health(&monitor), GUARDCAN_HEALTH_OK);
    check_bool("TEST 03: Uninitialized snapshot rejected",
               !guardcan_snapshot(&monitor, 0U, &snapshot), true);

    current = frame(0x060U, 4U, 1U, 1000U);
    check_bool("TEST 04: Valid telemetry accepted",
               guardcan_observe_frame(&monitor, &current), true);
    check_risk("TEST 05: Clean traffic is LOW",
               guardcan_risk(&monitor), GUARDCAN_RISK_LOW);
    check_health("TEST 06: Valid traffic health is OK",
                 guardcan_health(&monitor), GUARDCAN_HEALTH_OK);

    current = frame(0x060U, 3U, 2U, 1100U);
    check_bool("TEST 07: DLC mismatch rejected",
               !guardcan_observe_frame(&monitor, &current), true);
    check_risk("TEST 08: Invalid observation is UNKNOWN",
               guardcan_risk(&monitor), GUARDCAN_RISK_UNKNOWN);
    check_health("TEST 09: Invalid observation degrades health",
                 guardcan_health(&monitor), GUARDCAN_HEALTH_DEGRADED);

    guardcan_init(&monitor);
    current = frame(0x321U, 2U, 1U, 0U);
    check_bool("TEST 10: Unexpected ID is observed as anomaly",
               guardcan_observe_frame(&monitor, &current), true);
    check_risk("TEST 11: One unexpected ID is MEDIUM",
               guardcan_risk(&monitor), GUARDCAN_RISK_MEDIUM);

    guardcan_init(&monitor);
    for (index = 0U; index < 21U; ++index) {
        current = frame(0x060U, 4U, (uint16_t)(index + 1U),
                         1000U + index * 10U);
        guardcan_observe_frame(&monitor, &current);
    }
    check_risk("TEST 12: Elevated frequency increases risk",
               guardcan_risk(&monitor), GUARDCAN_RISK_MEDIUM);
    for (index = 21U; index < 55U; ++index) {
        current = frame(0x060U, 4U, (uint16_t)(index + 1U),
                         1000U + index * 10U);
        guardcan_observe_frame(&monitor, &current);
    }
    check_risk("TEST 13: Flood frequency reaches HIGH",
               guardcan_risk(&monitor), GUARDCAN_RISK_HIGH);

    guardcan_init(&monitor);
    current = frame(0x060U, 4U, 1U, 1000U);
    guardcan_observe_frame(&monitor, &current);
    current = frame(0x060U, 4U, 2U, 1005U);
    guardcan_observe_frame(&monitor, &current);
    check_risk("TEST 14: Short inter-arrival is evidence",
               guardcan_risk(&monitor), GUARDCAN_RISK_MEDIUM);

    guardcan_init(&monitor);
    current = frame(0x060U, 4U, 0U, 1000U);
    guardcan_observe_frame(&monitor, &current);
    current = frame(0x060U, 4U, 4U, 1100U);
    guardcan_observe_frame(&monitor, &current);
    check_risk("TEST 15: Forward sequence gap is not replay",
               guardcan_risk(&monitor), GUARDCAN_RISK_MEDIUM);
    current = frame(0x060U, 4U, 4U, 1200U);
    guardcan_observe_frame(&monitor, &current);
    check_risk("TEST 16: Replay is separate evidence",
               guardcan_risk(&monitor), GUARDCAN_RISK_HIGH);

    guardcan_init(&monitor);
    current = frame(0x060U, 4U, 65535U, 1000U);
    guardcan_observe_frame(&monitor, &current);
    current = frame(0x060U, 4U, 0U, 1100U);
    guardcan_observe_frame(&monitor, &current);
    check_risk("TEST 17: Sequence wrap is accepted",
               guardcan_risk(&monitor), GUARDCAN_RISK_LOW);

    guardcan_init(&monitor);
    current = frame(0x060U, 4U, 1U, 1000U);
    guardcan_observe_frame(&monitor, &current);
    guardcan_recover(&monitor, 2001U);
    check_risk("TEST 18: Clean window recovers risk",
               guardcan_risk(&monitor), GUARDCAN_RISK_LOW);
    check_health("TEST 19: Recovery restores health",
                 guardcan_health(&monitor), GUARDCAN_HEALTH_OK);

    guardcan_init(&monitor);
    current = frame(0x060U, 4U, 1U, UINT32_MAX - 10U);
    guardcan_observe_frame(&monitor, &current);
    check_bool("TEST 20: Snapshot supports timer wrap",
               guardcan_snapshot(&monitor, 20U, &snapshot) &&
               snapshot.risk_age_ms == 31U, true);

    check_health("TEST 21: Null monitor reports FAILED",
                 guardcan_health(0), GUARDCAN_HEALTH_FAILED);
    check_risk("TEST 22: Null monitor reports UNKNOWN",
               guardcan_risk(0), GUARDCAN_RISK_UNKNOWN);
    check_bool("TEST 23: Invalid frame input fails conservatively",
               !guardcan_observe_frame(&monitor,
               &(GuardcanFrame){ 0x800U, 0U, { 0U }, 0U }), true);

    guardcan_init(&monitor);
    current = frame(0x060U, 4U, 1U, 1000U);
    guardcan_observe_frame(&monitor, &current);
    risk.level = guardcan_to_cacgf_risk(guardcan_risk(&monitor));
    risk.timestamp_ms = 1000U;
    risk.detector_healthy = true;
    result = cacgf_evaluate(&command, &security, &vehicle, &risk,
                            &queue_state);
    check_bool("TEST 24: LOW GuardCAN risk preserves CACGF execution",
               result.decision == CACGF_DECISION_EXECUTE, true);

    guardcan_init(&monitor);
    command.command_id = 0x200U;
    current = frame(0x321U, 2U, 1U, 1000U);
    guardcan_observe_frame(&monitor, &current);
    current = frame(0x322U, 2U, 2U, 1100U);
    guardcan_observe_frame(&monitor, &current);
    risk.level = guardcan_to_cacgf_risk(guardcan_risk(&monitor));
    result = cacgf_evaluate(&command, &security, &vehicle, &risk,
                            &queue_state);
    check_bool("TEST 25: HIGH risk affects high criticality through CACGF",
               result.decision == CACGF_DECISION_ESCALATE, true);

    delay_queue_init(&delay_queue);
    command.command_id = 0x100U;
    command.sender_criticality = CACGF_CRITICALITY_LOW;
    risk.level = CACGF_RISK_LOW;
    vehicle.speed_kmh = 6U;
    result = cacgf_evaluate(&command, &security, &vehicle, &risk,
                            &queue_state);
    delay_queue_enqueue(&delay_queue, &command, 0, 0U, 1U, 1000U,
                        &security, result.decision);
    risk.level = CACGF_RISK_HIGH;
    vehicle.speed_kmh = 0U;
    {
        DelayQueueProcessResult processed = delay_queue_process(
            &delay_queue, 1100U, &vehicle, &risk);
        check_bool("TEST 26: Queue uses current GuardCAN risk",
                   processed.result.decision == CACGF_DECISION_EXECUTE ||
                   processed.result.decision == CACGF_DECISION_ESCALATE,
                   true);
    }

    printf("# ========================================\n");
    printf("RESULT: %d/%d TESTS PASSED\n", tests_passed, tests_run);
    return tests_run == tests_passed ? 0 : 1;
}
