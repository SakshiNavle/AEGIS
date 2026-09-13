#include "cacgf.h"
#include "command_policy.h"
#include "delay_queue.h"
#include "freshness.h"
#include "guardcan.h"
#include "vehicle_context.h"

#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_TRUE(condition)                                      \
    do {                                                            \
        ++tests_run;                                                \
        if (!(condition)) {                                         \
            printf("FAIL: %s (line %d)\n", #condition, __LINE__);  \
            return 0;                                               \
        }                                                           \
        ++tests_passed;                                             \
    } while (0)

#define ASSERT_EQ_INT(expected, actual)                             \
    do {                                                            \
        ++tests_run;                                                \
        if ((expected) != (actual)) {                               \
            printf("FAIL: %s == %s (line %d)\n",                  \
                   #expected, #actual, __LINE__);                   \
            return 0;                                               \
        }                                                           \
        ++tests_passed;                                             \
    } while (0)

static CacgfCommand make_command(uint32_t command_id)
{
    CacgfCommand command;

    memset(&command, 0, sizeof(command));
    command.command_id = command_id;

    return command;
}

static CacgfSecurityState valid_security(void)
{
    CacgfSecurityState security;

    memset(&security, 0, sizeof(security));
    security.authenticated = true;
    security.fresh = true;
    security.ttl_valid = true;

    return security;
}

static CacgfVehicleContext valid_vehicle(uint16_t speed,
                                         VehicleGear gear,
                                         bool brake_pressed)
{
    CacgfVehicleContext vehicle;

    memset(&vehicle, 0, sizeof(vehicle));
    vehicle.speed_kmh = speed;
    vehicle.gear = gear;
    vehicle.brake_pressed = brake_pressed;
    vehicle.telemetry_sequence = 1U;
    vehicle.telemetry_age_ms = 0U;
    vehicle.validity = CONTEXT_VALID;

    return vehicle;
}

static CacgfRiskState make_risk(CacgfRiskLevel level)
{
    CacgfRiskState risk;

    memset(&risk, 0, sizeof(risk));
    risk.level = level;

    return risk;
}

/* --------------------------------------------------------------- */
/* TEST 01: Normal validated command executes                     */
/* --------------------------------------------------------------- */

static int test_normal_command_executes(void)
{
    CacgfCommand command = make_command(0x100U);
    CacgfSecurityState security = valid_security();
    CacgfVehicleContext vehicle =
        valid_vehicle(0U, VEHICLE_GEAR_PARK, false);
    CacgfRiskState risk = make_risk(CACGF_RISK_LOW);
    CacgfQueueState queue = { false };
    CacgfResult result;

    result = cacgf_evaluate(&command, &security,
                            &vehicle, &risk, &queue);

    ASSERT_EQ_INT(CACGF_DECISION_EXECUTE, result.decision);
    ASSERT_EQ_INT(CACGF_SUCCESS_EXECUTE, result.reason);

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 02: Moving vehicle causes delay                            */
/* --------------------------------------------------------------- */

static int test_moving_command_enters_delay_queue(void)
{
    CacgfCommand command = make_command(0x100U);
    CacgfSecurityState security = valid_security();
    CacgfVehicleContext vehicle =
        valid_vehicle(20U, VEHICLE_GEAR_DRIVE, false);
    CacgfRiskState risk = make_risk(CACGF_RISK_LOW);
    CacgfQueueState queue_state = { false };
    DelayQueue queue;
    uint8_t payload[] = { 0x01U, 0x00U };
    CacgfResult decision;
    CacgfResult queued;

    delay_queue_init(&queue);

    decision = cacgf_evaluate(&command, &security,
                              &vehicle, &risk,
                              &queue_state);

    ASSERT_EQ_INT(CACGF_DECISION_DELAY, decision.decision);
    ASSERT_EQ_INT(CACGF_INFO_QUEUE_DELAYED, decision.reason);

    queued = delay_queue_enqueue(&queue,
                                 &command,
                                 payload,
                                 sizeof(payload),
                                 1U,
                                 1000U,
                                 &security,
                                 decision.decision);

    ASSERT_EQ_INT(CACGF_DECISION_DELAY, queued.decision);
    ASSERT_EQ_INT(1, delay_queue_size(&queue));

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 03: Delayed command re-evaluates and executes              */
/* --------------------------------------------------------------- */

static int test_delayed_command_executes_after_recovery(void)
{
    CacgfCommand command = make_command(0x100U);
    CacgfSecurityState security = valid_security();
    CacgfRiskState risk = make_risk(CACGF_RISK_LOW);
    VehicleContextStore store;
    VehicleTelemetry telemetry;
    DelayQueue queue;
    CacgfResult decision;
    CacgfResult queued;
    DelayQueueProcessResult processed;
    uint8_t payload[] = { 0x01U, 0x00U };

    vehicle_context_init(&store);
    delay_queue_init(&queue);

    memset(&telemetry, 0, sizeof(telemetry));

    /* Vehicle is moving. */
    telemetry.speed_kmh = 20U;
    telemetry.gear = VEHICLE_GEAR_DRIVE;
    telemetry.brake_pressed = false;
    telemetry.telemetry_sequence = 1U;
    telemetry.telemetry_age_ms = 0U;

    ASSERT_EQ_INT(CONTEXT_VALID,
                  vehicle_context_update(&store, &telemetry));

    decision = cacgf_evaluate(&command,
                              &security,
                              vehicle_context_current(&store),
                              &risk,
                              &(CacgfQueueState){ false });

    ASSERT_EQ_INT(CACGF_DECISION_DELAY, decision.decision);

    queued = delay_queue_enqueue(&queue,
                                 &command,
                                 payload,
                                 sizeof(payload),
                                 1U,
                                 1000U,
                                 &security,
                                 decision.decision);

    ASSERT_EQ_INT(CACGF_DECISION_DELAY, queued.decision);
    ASSERT_EQ_INT(1, delay_queue_size(&queue));

    /* Vehicle becomes stationary. */
    telemetry.speed_kmh = 0U;
    telemetry.gear = VEHICLE_GEAR_PARK;
    telemetry.telemetry_sequence = 2U;
    telemetry.telemetry_age_ms = 0U;

    ASSERT_EQ_INT(CONTEXT_VALID,
                  vehicle_context_update(&store, &telemetry));

    processed = delay_queue_process(
        &queue,
        1100U,
        vehicle_context_current(&store),
        &risk);

    ASSERT_EQ_INT(CACGF_DECISION_EXECUTE,
                  processed.result.decision);

    ASSERT_EQ_INT(CACGF_SUCCESS_EXECUTE,
                  processed.result.reason);

    ASSERT_TRUE(processed.removed);
    ASSERT_EQ_INT(0, delay_queue_size(&queue));

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 04: Delayed command expires                                */
/* --------------------------------------------------------------- */

static int test_delayed_command_expires(void)
{
    CacgfCommand command = make_command(0x100U);
    CacgfSecurityState security = valid_security();
    CacgfVehicleContext vehicle =
        valid_vehicle(20U, VEHICLE_GEAR_DRIVE, false);
    CacgfRiskState risk = make_risk(CACGF_RISK_LOW);
    DelayQueue queue;
    CacgfResult decision;
    CacgfResult queued;
    DelayQueueProcessResult processed;
    uint8_t payload[] = { 0x01U, 0x00U };

    delay_queue_init(&queue);

    decision = cacgf_evaluate(&command,
                              &security,
                              &vehicle,
                              &risk,
                              &(CacgfQueueState){ false });

    ASSERT_EQ_INT(CACGF_DECISION_DELAY, decision.decision);

    queued = delay_queue_enqueue(&queue,
                                 &command,
                                 payload,
                                 sizeof(payload),
                                 1U,
                                 1000U,
                                 &security,
                                 decision.decision);

    ASSERT_EQ_INT(CACGF_DECISION_DELAY, queued.decision);

    /*
     * Door Unlock has a local policy TTL of 200 ms.
     * 1201 - 1000 = 201 ms -> expired.
     */
    processed = delay_queue_process(&queue,
                                    1201U,
                                    &vehicle,
                                    &risk);

    ASSERT_EQ_INT(CACGF_DECISION_REJECT,
                  processed.result.decision);

    ASSERT_EQ_INT(CACGF_ERR_TTL_EXPIRED,
                  processed.result.reason);

    ASSERT_TRUE(processed.removed);
    ASSERT_EQ_INT(0, delay_queue_size(&queue));

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 05: Replay is rejected by freshness layer                  */
/* --------------------------------------------------------------- */

static int test_replay_is_rejected(void)
{
    FreshnessState state;
    FreshnessResult first;
    FreshnessResult replay;

    freshness_init(&state);

    first = freshness_validate_and_accept(
        &state, 100U, 0U, 200U);

    replay = freshness_validate_and_accept(
        &state, 100U, 0U, 200U);

    ASSERT_TRUE(freshness_result_is_accepted(first));
    ASSERT_EQ_INT(FRESHNESS_REPLAY, replay);

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 06: Stale vehicle context is rejected                      */
/* --------------------------------------------------------------- */

static int test_stale_vehicle_context_is_rejected(void)
{
    CacgfCommand command = make_command(0x100U);
    CacgfSecurityState security = valid_security();
    CacgfVehicleContext vehicle =
        valid_vehicle(0U, VEHICLE_GEAR_PARK, false);
    CacgfRiskState risk = make_risk(CACGF_RISK_LOW);
    CacgfQueueState queue = { false };
    CacgfResult result;

    vehicle.telemetry_age_ms =
        VEHICLE_CONTEXT_TELEMETRY_TTL_MS + 1U;

    vehicle.validity = CONTEXT_STALE;

    result = cacgf_evaluate(&command,
                            &security,
                            &vehicle,
                            &risk,
                            &queue);

    ASSERT_EQ_INT(CACGF_DECISION_REJECT, result.decision);
    ASSERT_EQ_INT(CACGF_ERR_STALE_CONTEXT, result.reason);

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 07: High risk + high criticality escalates                 */
/* --------------------------------------------------------------- */

static int test_high_risk_high_criticality_escalates(void)
{
    CacgfCommand command = make_command(0x200U);
    CacgfSecurityState security = valid_security();
    CacgfVehicleContext vehicle =
        valid_vehicle(0U, VEHICLE_GEAR_PARK, true);
    CacgfRiskState risk = make_risk(CACGF_RISK_HIGH);
    CacgfQueueState queue = { false };
    CacgfResult result;

    result = cacgf_evaluate(&command,
                            &security,
                            &vehicle,
                            &risk,
                            &queue);

    ASSERT_EQ_INT(CACGF_DECISION_ESCALATE, result.decision);
    ASSERT_EQ_INT(CACGF_ERR_HIGH_RISK, result.reason);

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 08: GuardCAN HIGH risk reaches CACGF                       */
/* --------------------------------------------------------------- */

static int test_guardcan_risk_flows_into_cacgf(void)
{
    GuardcanMonitor monitor;
    GuardcanSnapshot snapshot;
    GuardcanFrame frame;
    CacgfRiskState risk;
    CacgfCommand command = make_command(0x200U);
    CacgfSecurityState security = valid_security();
    CacgfVehicleContext vehicle =
        valid_vehicle(0U, VEHICLE_GEAR_PARK, true);
    CacgfQueueState queue = { false };
    CacgfResult result;
    unsigned int i;

    guardcan_init(&monitor);

    memset(&frame, 0, sizeof(frame));

    frame.can_id = 0x060U;
    frame.dlc = 4U;

    /*
     * 51 valid frames in one 1-second window.
     * This crosses GuardCAN's HIGH message threshold.
     */
    for (i = 0U; i < 51U; ++i) {
        uint16_t sequence = (uint16_t)(i + 1U);

        frame.timestamp_ms = 1000U + i;
        frame.payload[0] = (uint8_t)(sequence & 0xFFU);
        frame.payload[1] = (uint8_t)(sequence >> 8);
        frame.payload[2] = 0U;
        frame.payload[3] = 0U;

        ASSERT_TRUE(guardcan_observe_frame(&monitor, &frame));
    }

    ASSERT_TRUE(guardcan_snapshot(&monitor,
                                  1051U,
                                  &snapshot));

    ASSERT_EQ_INT(GUARDCAN_RISK_HIGH, snapshot.risk);
    ASSERT_EQ_INT(GUARDCAN_HEALTH_OK, snapshot.health);

    risk.level = guardcan_to_cacgf_risk(snapshot.risk);

    result = cacgf_evaluate(&command,
                            &security,
                            &vehicle,
                            &risk,
                            &queue);

    ASSERT_EQ_INT(CACGF_DECISION_ESCALATE,
                  result.decision);

    ASSERT_EQ_INT(CACGF_ERR_HIGH_RISK,
                  result.reason);

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 09: GuardCAN recovery restores LOW risk                    */
/* --------------------------------------------------------------- */

static int test_guardcan_recovery_restores_low_risk(void)
{
    GuardcanMonitor monitor;
    GuardcanSnapshot snapshot;
    GuardcanFrame frame;
    unsigned int i;

    guardcan_init(&monitor);

    memset(&frame, 0, sizeof(frame));

    frame.can_id = 0x060U;
    frame.dlc = 4U;

    for (i = 0U; i < 51U; ++i) {
        uint16_t sequence = (uint16_t)(i + 1U);

        frame.timestamp_ms = 1000U + i;
        frame.payload[0] = (uint8_t)(sequence & 0xFFU);
        frame.payload[1] = (uint8_t)(sequence >> 8);
        frame.payload[2] = 0U;
        frame.payload[3] = 0U;

        ASSERT_TRUE(guardcan_observe_frame(&monitor, &frame));
    }

    ASSERT_TRUE(guardcan_snapshot(&monitor,
                                  1051U,
                                  &snapshot));

    ASSERT_EQ_INT(GUARDCAN_RISK_HIGH, snapshot.risk);

    /*
     * More than one GuardCAN observation window without
     * new anomalies -> recovery.
     */
    guardcan_recover(&monitor, 2052U);

    ASSERT_TRUE(guardcan_snapshot(&monitor,
                                  2052U,
                                  &snapshot));

    ASSERT_EQ_INT(GUARDCAN_RISK_LOW, snapshot.risk);
    ASSERT_EQ_INT(GUARDCAN_HEALTH_OK, snapshot.health);
    ASSERT_EQ_INT(0, snapshot.evidence_score);

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 10: Delay queue is strictly bounded                        */
/* --------------------------------------------------------------- */

static int test_queue_is_bounded(void)
{
    CacgfCommand command = make_command(0x100U);
    CacgfSecurityState security = valid_security();
    DelayQueue queue;
    uint8_t payload[] = { 0x01U, 0x00U };
    CacgfResult result;
    unsigned int i;

    delay_queue_init(&queue);

    for (i = 0U; i < DELAY_QUEUE_CAPACITY; ++i) {
        result = delay_queue_enqueue(
            &queue,
            &command,
            payload,
            sizeof(payload),
            (uint16_t)(i + 1U),
            1000U + i,
            &security,
            CACGF_DECISION_DELAY);

        ASSERT_EQ_INT(CACGF_DECISION_DELAY,
                      result.decision);
    }

    ASSERT_EQ_INT(DELAY_QUEUE_CAPACITY,
                  delay_queue_size(&queue));

    result = delay_queue_enqueue(
        &queue,
        &command,
        payload,
        sizeof(payload),
        99U,
        2000U,
        &security,
        CACGF_DECISION_DELAY);

    ASSERT_EQ_INT(CACGF_DECISION_REJECT,
                  result.decision);

    ASSERT_EQ_INT(CACGF_ERR_QUEUE_FULL,
                  result.reason);

    ASSERT_EQ_INT(DELAY_QUEUE_CAPACITY,
                  delay_queue_size(&queue));

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 11: Detector UNKNOWN is conservative for critical command */
/* --------------------------------------------------------------- */

static int test_unknown_risk_rejects_high_criticality(void)
{
    CacgfCommand command = make_command(0x200U);
    CacgfSecurityState security = valid_security();
    CacgfVehicleContext vehicle =
        valid_vehicle(0U, VEHICLE_GEAR_PARK, true);
    CacgfRiskState risk = make_risk(CACGF_RISK_UNKNOWN);
    CacgfQueueState queue = { false };
    CacgfResult result;

    result = cacgf_evaluate(&command,
                            &security,
                            &vehicle,
                            &risk,
                            &queue);

    ASSERT_EQ_INT(CACGF_DECISION_REJECT,
                  result.decision);

    ASSERT_EQ_INT(CACGF_ERR_DETECTOR_UNKNOWN,
                  result.reason);

    return 1;
}

/* --------------------------------------------------------------- */
/* TEST 12: Non-delayable safety command never enters queue        */
/* --------------------------------------------------------------- */

static int test_safety_command_not_queued(void)
{
    CacgfCommand command = make_command(0x200U);
    CacgfSecurityState security = valid_security();
    CacgfVehicleContext vehicle =
        valid_vehicle(20U, VEHICLE_GEAR_DRIVE, false);
    CacgfRiskState risk = make_risk(CACGF_RISK_LOW);
    CacgfQueueState queue_state = { false };
    DelayQueue queue;
    uint8_t payload[] = { 0x01U, 0x00U };
    CacgfResult decision;
    CacgfResult queued;

    delay_queue_init(&queue);

    /*
     * Braking Torque is HIGH criticality and non-delayable.
     * The current CACGF policy should therefore execute it
     * when security/context/risk are acceptable.
     */
    decision = cacgf_evaluate(&command,
                              &security,
                              &vehicle,
                              &risk,
                              &queue_state);

    ASSERT_EQ_INT(CACGF_DECISION_EXECUTE,
                  decision.decision);

    /*
     * Even if somebody attempts to insert it into the delay
     * queue, the queue's policy check must reject it.
     */
    queued = delay_queue_enqueue(
        &queue,
        &command,
        payload,
        sizeof(payload),
        1U,
        1000U,
        &security,
        CACGF_DECISION_DELAY);

    ASSERT_EQ_INT(CACGF_DECISION_REJECT,
                  queued.decision);

    ASSERT_EQ_INT(0, delay_queue_size(&queue));

    return 1;
}

/* --------------------------------------------------------------- */

int main(void)
{
    printf("AEGIS integration tests\n");
    printf("=======================\n");

    printf("\nTEST 01: Normal command executes\n");
    if (!test_normal_command_executes()) return 1;

    printf("TEST 02: Moving command enters delay queue\n");
    if (!test_moving_command_enters_delay_queue()) return 1;

    printf("TEST 03: Delayed command executes after recovery\n");
    if (!test_delayed_command_executes_after_recovery()) return 1;

    printf("TEST 04: Delayed command expires\n");
    if (!test_delayed_command_expires()) return 1;

    printf("TEST 05: Replay is rejected\n");
    if (!test_replay_is_rejected()) return 1;

    printf("TEST 06: Stale vehicle context is rejected\n");
    if (!test_stale_vehicle_context_is_rejected()) return 1;

    printf("TEST 07: High risk + high criticality escalates\n");
    if (!test_high_risk_high_criticality_escalates()) return 1;

    printf("TEST 08: GuardCAN risk reaches CACGF\n");
    if (!test_guardcan_risk_flows_into_cacgf()) return 1;

    printf("TEST 09: GuardCAN recovery restores LOW risk\n");
    if (!test_guardcan_recovery_restores_low_risk()) return 1;

    printf("TEST 10: Delay queue remains bounded\n");
    if (!test_queue_is_bounded()) return 1;

    printf("TEST 11: UNKNOWN detector risk is conservative\n");
    if (!test_unknown_risk_rejects_high_criticality()) return 1;

    printf("TEST 12: Safety command never enters delay queue\n");
    if (!test_safety_command_not_queued()) return 1;

    printf("\n=======================\n");
    printf("Integration assertions: %d/%d passed\n",
           tests_passed,
           tests_run);

    return tests_passed == tests_run ? 0 : 1;
}