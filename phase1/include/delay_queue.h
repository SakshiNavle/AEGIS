#ifndef AEGIS_DELAY_QUEUE_H
#define AEGIS_DELAY_QUEUE_H

#include "cacgf.h"
#include "freshness.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DELAY_QUEUE_CAPACITY 8U
#define DELAY_QUEUE_PAYLOAD_MAX 16U

typedef struct {
    CacgfCommand command;
    uint8_t payload[DELAY_QUEUE_PAYLOAD_MAX];
    uint8_t payload_length;
    uint16_t sequence;
    uint32_t accepted_at_ms;
    uint32_t ttl_ms;
    CacgfSecurityState security;
} DelayQueueEntry;

typedef struct {
    DelayQueueEntry entries[DELAY_QUEUE_CAPACITY];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} DelayQueue;

typedef struct {
    CacgfResult result;
    bool removed;
} DelayQueueProcessResult;

void delay_queue_init(DelayQueue *queue);
bool delay_queue_is_empty(const DelayQueue *queue);
bool delay_queue_is_full(const DelayQueue *queue);
uint8_t delay_queue_size(const DelayQueue *queue);

CacgfResult delay_queue_enqueue(DelayQueue *queue,
                                const CacgfCommand *command,
                                const uint8_t *payload,
                                uint8_t payload_length,
                                uint16_t sequence,
                                uint32_t accepted_at_ms,
                                const CacgfSecurityState *security,
                                CacgfDecision prior_decision);

bool delay_queue_front(const DelayQueue *queue, DelayQueueEntry *entry);
bool delay_queue_dequeue(DelayQueue *queue, DelayQueueEntry *entry);

DelayQueueProcessResult delay_queue_process(DelayQueue *queue,
                                            uint32_t now_ms,
                                            const VehicleContext *vehicle,
                                            const CacgfRiskState *risk);

#ifdef __cplusplus
}
#endif

#endif
