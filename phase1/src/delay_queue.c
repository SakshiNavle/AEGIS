#include "delay_queue.h"

#include "command_policy.h"

#include <string.h>

static CacgfResult queue_result(CacgfDecision decision, CacgfReason reason)
{
    CacgfResult value = { decision, reason };
    return value;
}

static bool queue_state_valid(const DelayQueue *queue)
{
    return queue != 0 && queue->head < DELAY_QUEUE_CAPACITY &&
           queue->tail < DELAY_QUEUE_CAPACITY &&
           queue->count <= DELAY_QUEUE_CAPACITY;
}

void delay_queue_init(DelayQueue *queue)
{
    if (queue != 0) {
        memset(queue, 0, sizeof(*queue));
    }
}

bool delay_queue_is_empty(const DelayQueue *queue)
{
    return queue_state_valid(queue) && queue->count == 0U;
}

bool delay_queue_is_full(const DelayQueue *queue)
{
    return queue_state_valid(queue) &&
           queue->count == DELAY_QUEUE_CAPACITY;
}

uint8_t delay_queue_size(const DelayQueue *queue)
{
    return queue_state_valid(queue) ? queue->count : 0U;
}

CacgfResult delay_queue_enqueue(DelayQueue *queue,
                                const CacgfCommand *command,
                                const uint8_t *payload,
                                uint8_t payload_length,
                                uint16_t sequence,
                                uint32_t accepted_at_ms,
                                const CacgfSecurityState *security,
                                CacgfDecision prior_decision)
{
    CommandPolicy policy;
    DelayQueueEntry *entry;

    if (!queue_state_valid(queue) || command == 0 || security == 0 ||
        (payload_length > 0U && payload == 0) ||
        payload_length > DELAY_QUEUE_PAYLOAD_MAX ||
        prior_decision != CACGF_DECISION_DELAY) {
        return queue_result(CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    }
    if (queue->count == DELAY_QUEUE_CAPACITY) {
        return queue_result(CACGF_DECISION_REJECT, CACGF_ERR_QUEUE_FULL);
    }
    if (!security->authenticated || !security->fresh ||
        !security->ttl_valid ||
        !command_policy_resolve(command->command_id, &policy) ||
        !policy.delayable) {
        return queue_result(CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    }

    entry = &queue->entries[queue->tail];
    entry->command = *command;
    memset(entry->payload, 0, sizeof(entry->payload));
    if (payload_length > 0U) {
        memcpy(entry->payload, payload, payload_length);
    }
    entry->payload_length = payload_length;
    entry->sequence = sequence;
    entry->accepted_at_ms = accepted_at_ms;
    entry->ttl_ms = policy.ttl_ms;
    entry->security = *security;
    entry->security.fresh = true;
    entry->security.ttl_valid = true;
    queue->tail = (uint8_t)((queue->tail + 1U) % DELAY_QUEUE_CAPACITY);
    ++queue->count;
    return queue_result(CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
}

bool delay_queue_front(const DelayQueue *queue, DelayQueueEntry *entry)
{
    if (!queue_state_valid(queue) || entry == 0 || queue->count == 0U) {
        return false;
    }

    *entry = queue->entries[queue->head];
    return true;
}

bool delay_queue_dequeue(DelayQueue *queue, DelayQueueEntry *entry)
{
    if (!queue_state_valid(queue) || queue->count == 0U) {
        return false;
    }

    if (entry != 0) {
        *entry = queue->entries[queue->head];
    }
    memset(&queue->entries[queue->head], 0, sizeof(queue->entries[queue->head]));
    queue->head = (uint8_t)((queue->head + 1U) % DELAY_QUEUE_CAPACITY);
    --queue->count;
    return true;
}

DelayQueueProcessResult delay_queue_process(DelayQueue *queue,
                                            uint32_t now_ms,
                                            const VehicleContext *vehicle,
                                            const CacgfRiskState *risk)
{
    DelayQueueEntry entry;
    CacgfQueueState queue_state;
    DelayQueueProcessResult output;
    CacgfResult current;
    FreshnessLifetime lifetime;

    output.result = queue_result(CACGF_DECISION_REJECT,
                                 CACGF_ERR_INVALID_STATE);
    output.removed = false;
    if (!queue_state_valid(queue) || vehicle == 0 || risk == 0) {
        return output;
    }
    if (queue->count == 0U) {
        output.result = queue_result(CACGF_DECISION_REJECT,
                                     CACGF_ERR_INVALID_STATE);
        return output;
    }
    if (!delay_queue_front(queue, &entry)) {
        return output;
    }

    freshness_lifetime_start(&lifetime, entry.accepted_at_ms);
    if (!freshness_lifetime_is_valid(&lifetime, now_ms, entry.ttl_ms)) {
        delay_queue_dequeue(queue, 0);
        output.result = queue_result(CACGF_DECISION_REJECT,
                                     CACGF_ERR_TTL_EXPIRED);
        output.removed = true;
        return output;
    }

    /* The current entry already owns its slot; re-evaluation is not an
       insertion attempt, so a full queue must not reject it solely for that. */
    queue_state.queue_full = false;
    current = cacgf_evaluate(&entry.command, &entry.security, vehicle,
                             risk, &queue_state);
    if (current.decision == CACGF_DECISION_DELAY) {
        output.result = current;
        return output;
    }

    delay_queue_dequeue(queue, 0);
    output.result = current;
    output.removed = true;
    return output;
}
