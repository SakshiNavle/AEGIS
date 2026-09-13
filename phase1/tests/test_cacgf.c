#include "cacgf.h"
#include "command_policy.h"

#include <stdio.h>

static int tests_run;
static int tests_passed;

static CacgfCommand command(uint32_t command_id, CacgfCriticality sender_criticality,
                            bool sender_delayable, uint32_t sender_ttl_ms)
{
    CacgfCommand value = {
        command_id, sender_criticality, sender_delayable, sender_ttl_ms
    };
    return value;
}

static CacgfSecurityState security(bool authenticated, bool fresh, bool ttl_valid)
{
    CacgfSecurityState value = { authenticated, fresh, ttl_valid };
    return value;
}

static CacgfVehicleContext vehicle(uint16_t speed, bool valid)
{
    CacgfVehicleContext value = {
        speed, VEHICLE_GEAR_PARK, false, 1U, 0U,
        valid ? CONTEXT_VALID : CONTEXT_STALE
    };
    return value;
}

static CacgfRiskState risk(CacgfRiskLevel level)
{
    CacgfRiskState value = { level, 1000U, true };
    return value;
}

static CacgfQueueState queue(bool full)
{
    CacgfQueueState value = { full };
    return value;
}

static void run_test(const char *name,
                     const CacgfCommand *command_value,
                     const CacgfSecurityState *security_value,
                     const CacgfVehicleContext *vehicle_value,
                     const CacgfRiskState *risk_value,
                     const CacgfQueueState *queue_value,
                     CacgfDecision expected_decision,
                     CacgfReason expected_reason)
{
    CacgfResult actual;
    int passed;

    ++tests_run;
    actual = cacgf_evaluate(command_value, security_value, vehicle_value,
                            risk_value, queue_value);
    passed = actual.decision == expected_decision &&
             actual.reason == expected_reason;

    printf("%s\nExpected: %s / %s\nActual:   %s / %s\n%s\n\n",
           name,
           cacgf_decision_name(expected_decision),
           cacgf_reason_name(expected_reason),
           cacgf_decision_name(actual.decision),
           cacgf_reason_name(actual.reason),
           passed ? "PASS" : "FAIL");
    if (passed) {
        ++tests_passed;
    }
}

int main(void)
{
    CacgfCommand low = command(0x100U, CACGF_CRITICALITY_LOW, true, 200U);
    CacgfCommand medium_sender = command(0x200U, CACGF_CRITICALITY_MEDIUM, true, 1U);
    CacgfCommand high = command(0x200U, CACGF_CRITICALITY_HIGH, false, 200U);
    CacgfSecurityState valid_security = security(true, true, true);
    CacgfVehicleContext stopped = vehicle(0U, true);
    CacgfRiskState low_risk = risk(CACGF_RISK_LOW);
    CacgfQueueState available = queue(false);

    printf("# ========================================\n");
    printf("AEGIS CACGF PHASE 1 TEST\n\n");

    run_test("TEST 01: Valid low-criticality command",
             &low, &valid_security, &stopped, &low_risk, &available,
             CACGF_DECISION_EXECUTE, CACGF_SUCCESS_EXECUTE);
    stopped.speed_kmh = 50U;
    run_test("TEST 02: Moving vehicle",
             &low, &valid_security, &stopped, &low_risk, &available,
             CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    run_test("TEST 03: Moving command with full queue",
             &low, &valid_security, &stopped, &low_risk, &(CacgfQueueState){ true },
             CACGF_DECISION_REJECT, CACGF_ERR_QUEUE_FULL);
    run_test("TEST 04: Authentication failure",
             &low, &(CacgfSecurityState){ false, true, true }, &stopped,
             &low_risk, &available, CACGF_DECISION_REJECT, CACGF_ERR_AUTH_FAIL);
    run_test("TEST 05: Replay/stale command",
             &low, &(CacgfSecurityState){ true, false, true }, &stopped,
             &low_risk, &available, CACGF_DECISION_REJECT, CACGF_ERR_REPLAY);
    run_test("TEST 06: Expired command",
             &low, &(CacgfSecurityState){ true, true, false }, &stopped,
             &low_risk, &available, CACGF_DECISION_REJECT, CACGF_ERR_TTL_EXPIRED);
    run_test("TEST 07: Stale vehicle context",
             &low, &valid_security,
             &(CacgfVehicleContext){ 0U, VEHICLE_GEAR_PARK, false, 1U, 0U,
                                    CONTEXT_STALE },
             &low_risk, &available, CACGF_DECISION_REJECT, CACGF_ERR_STALE_CONTEXT);
    run_test("TEST 08: High-risk high-criticality command",
             &high, &valid_security, &stopped, &(CacgfRiskState){ CACGF_RISK_HIGH, 1000U, true },
             &available, CACGF_DECISION_ESCALATE, CACGF_ERR_HIGH_RISK);
    run_test("TEST 09: Unknown detector medium criticality",
             &medium_sender, &valid_security, &stopped,
             &(CacgfRiskState){ CACGF_RISK_UNKNOWN, 1000U, false }, &available,
             CACGF_DECISION_REJECT, CACGF_ERR_DETECTOR_UNKNOWN);
    stopped.speed_kmh = 0U;
    run_test("TEST 10: Unknown detector low criticality",
             &low, &valid_security, &stopped,
             &(CacgfRiskState){ CACGF_RISK_UNKNOWN, 1000U, false }, &available,
             CACGF_DECISION_EXECUTE, CACGF_SUCCESS_EXECUTE);
    run_test("TEST 11: Valid high-criticality command",
             &high, &valid_security, &stopped, &low_risk, &available,
             CACGF_DECISION_EXECUTE, CACGF_SUCCESS_EXECUTE);
    run_test("TEST 12: Multiple invalid conditions",
             &low, &(CacgfSecurityState){ false, false, false }, &stopped,
             &low_risk, &available, CACGF_DECISION_REJECT, CACGF_ERR_AUTH_FAIL);

    stopped.speed_kmh = CACGF_MAX_SPEED_KMH;
    run_test("EDGE 13: Speed exactly 5 km/h",
             &low, &valid_security, &stopped, &low_risk, &available,
             CACGF_DECISION_EXECUTE, CACGF_SUCCESS_EXECUTE);
    stopped.speed_kmh = CACGF_MAX_SPEED_KMH + 1U;
    run_test("EDGE 14: Speed slightly above 5 km/h",
             &low, &valid_security, &stopped, &low_risk, &available,
             CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    stopped.speed_kmh = CACGF_MAX_SPEED_KMH - 1U;
    run_test("EDGE 15: Speed slightly below 5 km/h",
             &low, &valid_security, &stopped, &low_risk, &available,
             CACGF_DECISION_EXECUTE, CACGF_SUCCESS_EXECUTE);
    run_test("EDGE 16: High risk low criticality",
             &low, &valid_security, &stopped,
             &(CacgfRiskState){ CACGF_RISK_HIGH, 1000U, true }, &available,
             CACGF_DECISION_EXECUTE, CACGF_SUCCESS_EXECUTE);
    run_test("EDGE 17: Medium risk high criticality",
             &high, &valid_security, &stopped,
             &(CacgfRiskState){ CACGF_RISK_MEDIUM, 1000U, true }, &available,
             CACGF_DECISION_EXECUTE, CACGF_SUCCESS_EXECUTE);
    run_test("EDGE 18: Unknown risk high criticality",
             &high, &valid_security, &stopped,
             &(CacgfRiskState){ CACGF_RISK_UNKNOWN, 1000U, false }, &available,
             CACGF_DECISION_REJECT, CACGF_ERR_DETECTOR_UNKNOWN);
    run_test("EDGE 19: Invalid enum state",
             &low, &valid_security,
             &(CacgfVehicleContext){ 0U, (VehicleGear)99, false, 1U, 0U,
                                    CONTEXT_VALID },
             &low_risk, &available,
             CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    run_test("EDGE 20: Unknown command",
             &(CacgfCommand){ 0x999U, CACGF_CRITICALITY_LOW, true, 200U },
             &valid_security, &stopped, &low_risk, &available,
             CACGF_DECISION_REJECT, CACGF_ERR_UNKNOWN_COMMAND);
    run_test("EDGE 21: Null command",
             0, &valid_security, &stopped, &low_risk, &available,
             CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);

    printf("POLICY TEST 01: Known 0x100 policy\n");
    {
        CommandPolicy policy;
        int passed = command_policy_resolve(0x100U, &policy) &&
                     policy.criticality == CACGF_CRITICALITY_LOW &&
                     policy.delayable && policy.ttl_ms == 200U;
        printf("%s\n\n", passed ? "PASS" : "FAIL");
        ++tests_run;
        if (passed) ++tests_passed;
    }
    printf("POLICY TEST 02: Known 0x150 policy\n");
    {
        CommandPolicy policy;
        int passed = command_policy_resolve(0x150U, &policy) &&
                     policy.criticality == CACGF_CRITICALITY_LOW &&
                     policy.delayable && policy.ttl_ms == 500U;
        printf("%s\n\n", passed ? "PASS" : "FAIL");
        ++tests_run;
        if (passed) ++tests_passed;
    }
    printf("POLICY TEST 03: Known 0x200 policy\n");
    {
        CommandPolicy policy;
        int passed = command_policy_resolve(0x200U, &policy) &&
                     policy.criticality == CACGF_CRITICALITY_HIGH &&
                     !policy.delayable && policy.ttl_ms == 200U;
        printf("%s\n\n", passed ? "PASS" : "FAIL");
        ++tests_run;
        if (passed) ++tests_passed;
    }
    printf("POLICY TEST 04: Known 0x250 policy\n");
    {
        CommandPolicy policy;
        int passed = command_policy_resolve(0x250U, &policy) &&
                     policy.criticality == CACGF_CRITICALITY_HIGH &&
                     !policy.delayable && policy.ttl_ms == 200U;
        printf("%s\n\n", passed ? "PASS" : "FAIL");
        ++tests_run;
        if (passed) ++tests_passed;
    }
    printf("POLICY TEST 05: Unknown command\n");
    {
        CommandPolicy policy;
        int passed = !command_policy_resolve(0x999U, &policy);
        printf("%s\n\n", passed ? "PASS" : "FAIL");
        ++tests_run;
        if (passed) ++tests_passed;
    }
    printf("POLICY TEST 06: Sender criticality cannot override local policy\n");
    {
        CacgfCommand sender_override = command(0x100U, CACGF_CRITICALITY_HIGH, true, 200U);
        CacgfResult actual = cacgf_evaluate(&sender_override, &valid_security,
                                            &stopped, &low_risk, &available);
        int passed = actual.decision == CACGF_DECISION_EXECUTE &&
                     actual.reason == CACGF_SUCCESS_EXECUTE;
        printf("%s\n\n", passed ? "PASS" : "FAIL");
        ++tests_run;
        if (passed) ++tests_passed;
    }
    printf("POLICY TEST 07: Sender TTL cannot override local policy\n");
    {
        CacgfCommand sender_override = command(0x150U, CACGF_CRITICALITY_LOW, true, 1U);
        CommandPolicy policy;
        int passed = command_policy_resolve(sender_override.command_id, &policy) &&
                     policy.ttl_ms == 500U;
        printf("%s\n\n", passed ? "PASS" : "FAIL");
        ++tests_run;
        if (passed) ++tests_passed;
    }
    printf("POLICY TEST 08: Existing CACGF coverage retained\n");
    {
        CacgfResult actual = cacgf_evaluate(&high, &valid_security, &stopped,
                                            &low_risk, &available);
        int passed = actual.decision == CACGF_DECISION_EXECUTE &&
                     actual.reason == CACGF_SUCCESS_EXECUTE;
        printf("%s\n\n", passed ? "PASS" : "FAIL");
        ++tests_run;
        if (passed) ++tests_passed;
    }

    printf("# ========================================\n");
    printf("RESULT: %d/%d TESTS PASSED\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
