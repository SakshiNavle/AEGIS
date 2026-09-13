#include "delay_queue.h"

#include <stdio.h>
#include <string.h>

static int tests_run;
static int tests_passed;

static CacgfCommand command(uint32_t command_id)
{
    CacgfCommand value = {
        command_id, CACGF_CRITICALITY_HIGH, false, 1U
    };
    return value;
}

static CacgfSecurityState security(void)
{
    CacgfSecurityState value = { true, true, true };
    return value;
}

static CacgfRiskState risk(void)
{
    CacgfRiskState value = { CACGF_RISK_LOW, 0U, true };
    return value;
}

static VehicleContext context(uint16_t speed, VehicleGear gear)
{
    VehicleContext value = {
        speed, gear, false, 1U, 0U, CONTEXT_VALID
    };
    return value;
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

static void check_result(const char *name, CacgfResult actual,
                         CacgfDecision decision, CacgfReason reason)
{
    int passed = actual.decision == decision && actual.reason == reason;

    ++tests_run;
    printf("%s\nExpected: %s / %s\nActual:   %s / %s\n%s\n\n", name,
           cacgf_decision_name(decision), cacgf_reason_name(reason),
           cacgf_decision_name(actual.decision),
           cacgf_reason_name(actual.reason), passed ? "PASS" : "FAIL");
    if (passed) {
        ++tests_passed;
    }
}

static CacgfResult evaluate_delay(DelayQueue *queue, const CacgfCommand *cmd,
                                  const VehicleContext *vehicle,
                                  uint32_t accepted_at)
{
    CacgfSecurityState auth = security();
    CacgfRiskState current_risk = risk();
    CacgfQueueState queue_state = { false };
    CacgfResult decision = cacgf_evaluate(
        cmd, &auth, vehicle, &current_risk, &queue_state);

    if (decision.decision == CACGF_DECISION_DELAY) {
        return delay_queue_enqueue(queue, cmd, 0, 0U, 1U, accepted_at,
                                   &auth, decision.decision);
    }
    return decision;
}

int main(void)
{
    DelayQueue queue;
    DelayQueueEntry entry;
    DelayQueueProcessResult processed;
    CacgfCommand door = command(0x100U);
    CacgfCommand trunk = command(0x150U);
    CacgfCommand brake = command(0x200U);
    CacgfCommand power = command(0x250U);
    VehicleContext moving = context(6U, VEHICLE_GEAR_DRIVE);
    VehicleContext stopped = context(0U, VEHICLE_GEAR_PARK);
    CacgfSecurityState auth = security();
    CacgfRiskState low_risk = risk();
    uint8_t payload[2] = { 0xAAU, 0x55U };
    uint32_t index;

    printf("# ========================================\n");
    printf("AEGIS PHASE 4 DELAY QUEUE TEST\n\n");

    delay_queue_init(&queue);
    check_bool("TEST 01: Empty queue after initialization",
               delay_queue_is_empty(&queue), true);
    check_bool("TEST 02: Initial size is zero",
               delay_queue_size(&queue) == 0U, true);
    check_bool("TEST 03: Initial queue is not full",
               delay_queue_is_full(&queue), false);

    check_result("TEST 04: Enqueue first delayed command",
                 evaluate_delay(&queue, &door, &moving, 1000U),
                 CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    check_bool("TEST 05: Inspect front entry",
               delay_queue_front(&queue, &entry) &&
               entry.command.command_id == 0x100U, true);
    check_bool("TEST 06: Enqueue preserves payload and sequence",
               delay_queue_dequeue(&queue, &entry), true);
    check_bool("TEST 07: Dequeued entry matches command data",
               entry.command.command_id == 0x100U, true);

    check_result("TEST 08: Enqueue FIFO command 0x100",
                 evaluate_delay(&queue, &door, &moving, 1000U),
                 CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    check_result("TEST 09: Enqueue FIFO command 0x150",
                 evaluate_delay(&queue, &trunk, &moving, 1000U),
                 CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    check_bool("TEST 10: FIFO front is 0x100",
               delay_queue_front(&queue, &entry) &&
               entry.command.command_id == 0x100U, true);
    delay_queue_dequeue(&queue, &entry);
    check_bool("TEST 11: FIFO next is 0x150",
               delay_queue_front(&queue, &entry) &&
               entry.command.command_id == 0x150U, true);
    delay_queue_dequeue(&queue, 0);

    for (index = 0U; index < DELAY_QUEUE_CAPACITY; ++index) {
        check_result("TEST 12: Fill bounded queue",
                     evaluate_delay(&queue, &door, &moving, 1000U),
                     CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    }
    check_bool("TEST 13: Queue reports full at capacity",
               delay_queue_is_full(&queue) &&
               delay_queue_size(&queue) == DELAY_QUEUE_CAPACITY, true);
    check_result("TEST 14: Ninth enqueue is rejected",
                 evaluate_delay(&queue, &door, &moving, 1000U),
                 CACGF_DECISION_REJECT, CACGF_ERR_QUEUE_FULL);
    check_bool("TEST 15: Full queue retains front entry",
               delay_queue_front(&queue, &entry) &&
               entry.command.command_id == 0x100U, true);
    delay_queue_init(&queue);

    check_result("TEST 16: Process before TTL expiry remains delayed",
                 evaluate_delay(&queue, &door, &moving, 1000U),
                 CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    processed = delay_queue_process(&queue, 1100U, &moving, &low_risk);
    check_result("TEST 17: Unsafe current context remains delayed",
                 processed.result, CACGF_DECISION_DELAY,
                 CACGF_INFO_QUEUE_DELAYED);
    check_bool("TEST 18: Delayed command remains queued",
               !processed.removed && delay_queue_size(&queue) == 1U, true);
    processed = delay_queue_process(&queue, 1100U, &stopped, &low_risk);
    check_result("TEST 19: Current safe context executes",
                 processed.result, CACGF_DECISION_EXECUTE,
                 CACGF_SUCCESS_EXECUTE);
    check_bool("TEST 20: Executed command is removed",
               processed.removed && delay_queue_is_empty(&queue), true);

    delay_queue_init(&queue);
    evaluate_delay(&queue, &door, &moving, 1000U);
    processed = delay_queue_process(&queue, 1200U, &stopped, &low_risk);
    check_result("TEST 21: TTL boundary remains executable",
                 processed.result, CACGF_DECISION_EXECUTE,
                 CACGF_SUCCESS_EXECUTE);
    delay_queue_init(&queue);
    evaluate_delay(&queue, &door, &moving, 1000U);
    processed = delay_queue_process(&queue, 1201U, &stopped, &low_risk);
    check_result("TEST 22: Expired command is rejected",
                 processed.result, CACGF_DECISION_REJECT,
                 CACGF_ERR_TTL_EXPIRED);
    check_bool("TEST 23: Expired command never executes",
               processed.removed && delay_queue_is_empty(&queue), true);

    delay_queue_init(&queue);
    check_result("TEST 24: Non-delayable braking command rejected",
                 delay_queue_enqueue(&queue, &brake, 0, 0U, 1U, 1000U,
                                     &auth, CACGF_DECISION_DELAY),
                 CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    check_result("TEST 25: Non-delayable power command rejected",
                 delay_queue_enqueue(&queue, &power, 0, 0U, 1U, 1000U,
                                     &auth, CACGF_DECISION_DELAY),
                 CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    check_bool("TEST 26: Non-delayable commands never enter queue",
               delay_queue_is_empty(&queue), true);

    delay_queue_init(&queue);
    auth.fresh = false;
    check_result("TEST 27: Replayed command cannot be enqueued",
                 delay_queue_enqueue(&queue, &door, 0, 0U, 2U, 1000U,
                                     &auth, CACGF_DECISION_DELAY),
                 CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    auth.fresh = true;

    check_result("TEST 28: Payload is stored with entry",
                 delay_queue_enqueue(&queue, &door, payload, 2U, 10U, 1000U,
                                     &auth, CACGF_DECISION_DELAY),
                 CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    check_bool("TEST 29: Payload and sequence round-trip",
               delay_queue_front(&queue, &entry) &&
               entry.payload_length == 2U &&
               memcmp(entry.payload, payload, 2U) == 0 &&
               entry.sequence == 10U, true);

    delay_queue_init(&queue);
    check_result("TEST 30: Timer wrap remains within TTL",
                 delay_queue_enqueue(&queue, &door, 0, 0U, 1U,
                                     UINT32_MAX - 50U, &auth,
                                     CACGF_DECISION_DELAY),
                 CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    processed = delay_queue_process(&queue, 149U, &stopped, &low_risk);
    check_result("TEST 31: Wrapped elapsed time executes before expiry",
                 processed.result, CACGF_DECISION_EXECUTE,
                 CACGF_SUCCESS_EXECUTE);

    check_result("TEST 32: Null queue rejected",
                 delay_queue_enqueue(0, &door, 0, 0U, 1U, 0U, &auth,
                                     CACGF_DECISION_DELAY),
                 CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    check_result("TEST 33: Null command rejected",
                 delay_queue_enqueue(&queue, 0, 0, 0U, 1U, 0U, &auth,
                                     CACGF_DECISION_DELAY),
                 CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    check_bool("TEST 34: Null process inputs are conservative",
               delay_queue_process(&queue, 0U, 0, &low_risk).result.decision ==
               CACGF_DECISION_REJECT, true);

    delay_queue_init(&queue);
    queue.count = DELAY_QUEUE_CAPACITY + 1U;
    check_bool("TEST 35: Corrupt queue state is not executable",
               delay_queue_size(&queue) == 0U &&
               !delay_queue_is_full(&queue), true);

    printf("# ========================================\n");
    printf("RESULT: %d/%d TESTS PASSED\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
