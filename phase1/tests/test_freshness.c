#include "cacgf.h"
#include "command_policy.h"
#include "freshness.h"

#include <stdio.h>

static int tests_run;
static int tests_passed;

static void check_result(const char *name, FreshnessResult actual,
                         FreshnessResult expected)
{
    int passed = actual == expected;

    ++tests_run;
    printf("%s\nExpected: %s\nActual:   %s\n%s\n\n",
           name, freshness_result_name(expected), freshness_result_name(actual),
           passed ? "PASS" : "FAIL");
    if (passed) {
        ++tests_passed;
    }
}

static void check_bool(const char *name, bool actual, bool expected)
{
    int passed = actual == expected;

    ++tests_run;
    printf("%s\nExpected: %s\nActual:   %s\n%s\n\n",
           name, expected ? "true" : "false", actual ? "true" : "false",
           passed ? "PASS" : "FAIL");
    if (passed) {
        ++tests_passed;
    }
}

int main(void)
{
    FreshnessState state;
    FreshnessLifetime lifetime;
    CommandPolicy policy;
    FreshnessResult result;

    printf("# ========================================\n");
    printf("AEGIS PHASE 2 FRESHNESS TEST\n\n");

    freshness_init(&state);
    check_result("TEST 01: First sequence accepted",
                 freshness_validate_and_accept(&state, 10U, 100U, 200U),
                 FRESHNESS_FIRST_ACCEPTED);
    check_result("TEST 02: Equal sequence rejected as replay",
                 freshness_validate_and_accept(&state, 10U, 100U, 200U),
                 FRESHNESS_REPLAY);
    check_result("TEST 03: Older sequence rejected",
                 freshness_validate_and_accept(&state, 9U, 100U, 200U),
                 FRESHNESS_REPLAY);
    check_result("TEST 04: Newer sequence accepted",
                 freshness_validate_and_accept(&state, 11U, 100U, 200U),
                 FRESHNESS_ACCEPTED);
    check_result("TEST 05: Forward sequence gap accepted",
                 freshness_validate_and_accept(&state, 20U, 100U, 200U),
                 FRESHNESS_SEQUENCE_GAP);

    freshness_reset(&state);
    check_result("TEST 06: 65535 to 0 wraparound accepted",
                 freshness_validate_and_accept(&state, 65535U, 0U, 200U),
                 FRESHNESS_FIRST_ACCEPTED);
    check_result("TEST 06b: Wrapped sequence 0 accepted",
                 freshness_validate_and_accept(&state, 0U, 0U, 200U),
                 FRESHNESS_ACCEPTED);
    check_result("TEST 07: 0 to 65535 is older",
                 freshness_validate_and_accept(&state, 65535U, 0U, 200U),
                 FRESHNESS_REPLAY);

    freshness_reset(&state);
    check_result("TEST 08: TTL exactly at boundary",
                 freshness_validate_and_accept(&state, 1U, 200U, 200U),
                 FRESHNESS_FIRST_ACCEPTED);
    check_result("TEST 09: TTL exceeded",
                 freshness_validate_and_accept(&state, 2U, 201U, 200U),
                 FRESHNESS_TTL_EXPIRED);
    check_result("TEST 09b: Expired sequence does not advance state",
                 freshness_validate_and_accept(&state, 1U, 0U, 200U),
                 FRESHNESS_REPLAY);

    check_result("TEST 10: Null freshness state is invalid",
                 freshness_validate_and_accept(0, 1U, 0U, 200U),
                 FRESHNESS_INVALID_STATE);
    freshness_reset(&state);
    check_result("TEST 11: Sequence state reset",
                 freshness_validate_and_accept(&state, 1U, 0U, 200U),
                 FRESHNESS_FIRST_ACCEPTED);
    freshness_reset(&state);
    check_result("TEST 11b: Reset permits prior sequence again",
                 freshness_validate_and_accept(&state, 1U, 0U, 200U),
                 FRESHNESS_FIRST_ACCEPTED);

    check_bool("TEST 12: Sender TTL cannot override 0x100 local policy",
               command_policy_resolve(0x100U, &policy) && policy.ttl_ms == 200U,
               true);
    check_bool("TEST 13: Sequence comparison wraps safely",
               freshness_sequence_is_newer(0U, 65535U), true);
    check_bool("TEST 14: Reverse wrap is not newer",
               freshness_sequence_is_newer(65535U, 0U), false);
    check_bool("TEST 15: Freshness acceptance maps to CACGF fresh state",
               freshness_result_is_accepted(FRESHNESS_SEQUENCE_GAP), true);

    result = freshness_validate_and_accept(&state, 2U, 0U, 200U);
    check_bool("TEST 16: Accepted freshness can feed CACGF security",
               freshness_result_is_accepted(result), true);
    freshness_lifetime_start(&lifetime, 1000U);
    check_bool("TEST 17: Lifetime valid at 1100 ms with 200 ms TTL",
               freshness_lifetime_is_valid(&lifetime, 1100U, 200U), true);
    check_bool("TEST 18: Lifetime valid exactly at TTL boundary",
               freshness_lifetime_is_valid(&lifetime, 1200U, 200U), true);
    check_bool("TEST 19: Lifetime expires after TTL boundary",
               freshness_lifetime_is_valid(&lifetime, 1201U, 200U), false);
    check_bool("TEST 20: Delayed command expires while waiting",
               freshness_lifetime_is_valid(&lifetime, 1201U, 200U), false);
    freshness_reset(&state);
    result = freshness_check_sequence(&state, 42U);
    check_bool("TEST 20b: Sequence can be checked before authentication",
               freshness_result_is_accepted(result), true);
    freshness_accept_sequence(&state, 42U);
    check_result("TEST 20c: Accepted sequence is recorded after authentication",
                 freshness_check_sequence(&state, 42U),
                 FRESHNESS_REPLAY);
    {
        CacgfCommand command = { 0x100U, CACGF_CRITICALITY_LOW, true, 1U };
        CacgfSecurityState security = { true, true, false };
        CacgfVehicleContext vehicle = {
            0U, VEHICLE_GEAR_PARK, false, 1U, 0U, CONTEXT_VALID
        };
        CacgfRiskState risk = { CACGF_RISK_LOW, 1000U, true };
        CacgfQueueState queue = { false };
        CacgfResult cacgf_result = cacgf_evaluate(
            &command, &security, &vehicle, &risk, &queue);
        check_bool("TEST 21: Expired lifetime cannot execute",
                   cacgf_result.decision == CACGF_DECISION_REJECT &&
                   cacgf_result.reason == CACGF_ERR_TTL_EXPIRED, true);
    }
    {
        CacgfCommand command = { 0x100U, CACGF_CRITICALITY_LOW, true, 200U };
        CacgfSecurityState security = { true, false, true };
        CacgfVehicleContext vehicle = {
            0U, VEHICLE_GEAR_PARK, false, 1U, 0U, CONTEXT_VALID
        };
        CacgfRiskState risk = { CACGF_RISK_LOW, 1000U, true };
        CacgfQueueState queue = { false };
        CacgfResult cacgf_result = cacgf_evaluate(
            &command, &security, &vehicle, &risk, &queue);
        check_bool("TEST 21b: Replay cannot execute with valid lifetime",
                   cacgf_result.decision == CACGF_DECISION_REJECT &&
                   cacgf_result.reason == CACGF_ERR_REPLAY, true);
    }
    {
        CacgfCommand command = { 0x100U, CACGF_CRITICALITY_HIGH, false, 1U };
        CacgfSecurityState security = { true, true, true };
        CacgfVehicleContext vehicle = {
            0U, VEHICLE_GEAR_PARK, false, 1U, 0U, CONTEXT_VALID
        };
        CacgfRiskState risk = { CACGF_RISK_LOW, 1000U, true };
        CacgfQueueState queue = { false };
        CacgfResult cacgf_result = cacgf_evaluate(
            &command, &security, &vehicle, &risk, &queue);
        check_bool("TEST 22: Fresh accepted command reaches CACGF",
                   cacgf_result.decision == CACGF_DECISION_EXECUTE &&
                   cacgf_result.reason == CACGF_SUCCESS_EXECUTE, true);
        security.ttl_valid = false;
        cacgf_result = cacgf_evaluate(
            &command, &security, &vehicle, &risk, &queue);
        check_bool("TEST 23: TTL failure reaches CACGF rejection",
                   cacgf_result.decision == CACGF_DECISION_REJECT &&
                   cacgf_result.reason == CACGF_ERR_TTL_EXPIRED, true);
    }

    printf("# ========================================\n");
    printf("RESULT: %d/%d TESTS PASSED\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
