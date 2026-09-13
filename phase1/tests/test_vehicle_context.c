#include "cacgf.h"
#include "vehicle_context.h"

#include <stdio.h>

static int tests_run;
static int tests_passed;

static VehicleTelemetry telemetry(uint16_t sequence, uint16_t speed,
                                 VehicleGear gear, uint32_t age)
{
    VehicleTelemetry value = {
        speed, gear, false, sequence, age
    };
    return value;
}

static void check_status(const char *name, VehicleContextValidity actual,
                         VehicleContextValidity expected)
{
    int passed = actual == expected;

    ++tests_run;
    printf("%s\nExpected: %s\nActual:   %s\n%s\n\n", name,
           vehicle_context_validity_name(expected),
           vehicle_context_validity_name(actual),
           passed ? "PASS" : "FAIL");
    if (passed) {
        ++tests_passed;
    }
}

static void check_bool(const char *name, bool actual, bool expected)
{
    int passed = actual == expected;

    ++tests_run;
    printf("%s\nExpected: %s\nActual:   %s\n%s\n\n", name,
           expected ? "true" : "false", actual ? "true" : "false",
           passed ? "PASS" : "FAIL");
    if (passed) {
        ++tests_passed;
    }
}

static CacgfResult evaluate_context(const VehicleContext *context,
                                    uint32_t command_id)
{
    CacgfCommand command = {
        command_id, CACGF_CRITICALITY_HIGH, false, 1U
    };
    CacgfSecurityState security = { true, true, true };
    CacgfRiskState risk = { CACGF_RISK_LOW, 0U, true };
    CacgfQueueState queue = { false };

    return cacgf_evaluate(&command, &security, context, &risk, &queue);
}

int main(void)
{
    VehicleContextStore store;
    VehicleTelemetry frame;
    const VehicleContext *context;
    CacgfResult result;

    printf("# ========================================\n");
    printf("AEGIS PHASE 3 VEHICLE CONTEXT TEST\n\n");

    vehicle_context_init(&store);
    context = vehicle_context_current(&store);
    check_status("TEST 01: Uninitialized context", context->validity,
                 CONTEXT_UNKNOWN);
    check_bool("TEST 02: Null update input is invalid",
               vehicle_context_update(&store, 0) == CONTEXT_INVALID, true);

    frame = telemetry(1U, 0U, VEHICLE_GEAR_PARK, 0U);
    check_status("TEST 03: Valid PARK initialization",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(2U, 0U, VEHICLE_GEAR_NEUTRAL, 0U);
    check_status("TEST 04: NEUTRAL context",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(3U, 6U, VEHICLE_GEAR_DRIVE, 0U);
    check_status("TEST 05: DRIVE context",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(4U, 6U, VEHICLE_GEAR_REVERSE, 0U);
    check_status("TEST 06: REVERSE context",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(5U, 0U, VEHICLE_GEAR_UNKNOWN, 0U);
    check_status("TEST 07: UNKNOWN gear is invalid",
                 vehicle_context_update(&store, &frame), CONTEXT_INVALID);
    frame = telemetry(6U, 0U, (VehicleGear)99, 0U);
    check_status("TEST 08: Invalid gear enum",
                 vehicle_context_update(&store, &frame), CONTEXT_INVALID);

    frame = telemetry(7U, 0U, VEHICLE_GEAR_PARK, 0U);
    check_status("TEST 09: Speed zero",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(8U, 5U, VEHICLE_GEAR_PARK, 0U);
    check_status("TEST 10: Speed five",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(9U, 6U, VEHICLE_GEAR_DRIVE, 0U);
    check_status("TEST 11: Speed six",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(10U, UINT16_MAX, VEHICLE_GEAR_DRIVE, 0U);
    check_status("TEST 12: Maximum representable speed",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);

    frame = telemetry(11U, 0U, VEHICLE_GEAR_PARK, 50U);
    check_status("TEST 13: Telemetry exactly at 50 ms",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(12U, 0U, VEHICLE_GEAR_PARK, 51U);
    check_status("TEST 14: Telemetry over 50 ms is stale",
                 vehicle_context_update(&store, &frame), CONTEXT_STALE);
    context = vehicle_context_current(&store);
    check_bool("TEST 15: Stale context is not policy-valid",
               vehicle_context_is_valid(context), false);

    frame = telemetry(13U, 0U, VEHICLE_GEAR_PARK, 0U);
    check_status("TEST 16: Valid context restored",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(13U, 0U, VEHICLE_GEAR_PARK, 0U);
    check_status("TEST 17: Replay telemetry rejected",
                 vehicle_context_update(&store, &frame), CONTEXT_STALE);
    check_bool("TEST 18: Replay cannot remain policy-valid",
               vehicle_context_is_valid(vehicle_context_current(&store)),
               false);

    vehicle_context_init(&store);
    frame = telemetry(65535U, 0U, VEHICLE_GEAR_PARK, 0U);
    check_status("TEST 19: Wrap setup at 65535",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(0U, 0U, VEHICLE_GEAR_PARK, 0U);
    check_status("TEST 20: Telemetry wrap 65535 to 0",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);
    frame = telemetry(65535U, 0U, VEHICLE_GEAR_PARK, 0U);
    check_status("TEST 21: Reverse wrap rejected",
                 vehicle_context_update(&store, &frame), CONTEXT_STALE);

    vehicle_context_init(&store);
    frame = telemetry(1U, 0U, VEHICLE_GEAR_PARK, 0U);
    vehicle_context_update(&store, &frame);
    frame = telemetry(3U, 6U, VEHICLE_GEAR_DRIVE, 0U);
    check_status("TEST 22: Forward sequence gap accepted",
                 vehicle_context_update(&store, &frame), CONTEXT_VALID);

    vehicle_context_init(&store);
    frame = telemetry(1U, 0U, VEHICLE_GEAR_PARK, 0U);
    vehicle_context_update(&store, &frame);
    context = vehicle_context_current(&store);
    frame = telemetry(2U, 99U, (VehicleGear)99, 0U);
    check_status("TEST 23: Malformed telemetry rejected",
                 vehicle_context_update(&store, &frame), CONTEXT_INVALID);
    check_bool("TEST 24: Malformed input retains diagnostics",
               context->speed_kmh == 0U &&
               context->telemetry_sequence == 1U, true);
    check_bool("TEST 25: Malformed input cannot become policy-valid",
               vehicle_context_is_valid(context), false);

    vehicle_context_init(&store);
    frame = telemetry(1U, 0U, VEHICLE_GEAR_PARK, 0U);
    vehicle_context_update(&store, &frame);
    context = vehicle_context_current(&store);
    result = evaluate_context(context, 0x100U);
    check_bool("TEST 26: Stationary Door Unlock executes",
               result.decision == CACGF_DECISION_EXECUTE, true);
    frame = telemetry(2U, 6U, VEHICLE_GEAR_DRIVE, 0U);
    vehicle_context_update(&store, &frame);
    result = evaluate_context(vehicle_context_current(&store), 0x100U);
    check_bool("TEST 27: Moving Door Unlock delays",
               result.decision == CACGF_DECISION_DELAY, true);
    frame = telemetry(3U, 0U, VEHICLE_GEAR_PARK, 51U);
    vehicle_context_update(&store, &frame);
    result = evaluate_context(vehicle_context_current(&store), 0x100U);
    check_bool("TEST 28: Stale context rejects Door Unlock",
               result.decision == CACGF_DECISION_REJECT &&
               result.reason == CACGF_ERR_STALE_CONTEXT, true);
    result = evaluate_context(&(VehicleContext){
        0U, VEHICLE_GEAR_UNKNOWN, false, 0U, 0U, CONTEXT_INVALID
    }, 0x200U);
    check_bool("TEST 29: Invalid context rejects high-criticality command",
               result.decision == CACGF_DECISION_REJECT &&
               result.reason == CACGF_ERR_STALE_CONTEXT, true);

    printf("# ========================================\n");
    printf("RESULT: %d/%d TESTS PASSED\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}