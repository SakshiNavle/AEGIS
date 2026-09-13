#ifndef AEGIS_FRESHNESS_H
#define AEGIS_FRESHNESS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FRESHNESS_INVALID_STATE = 0,
    FRESHNESS_FIRST_ACCEPTED,
    FRESHNESS_ACCEPTED,
    FRESHNESS_SEQUENCE_GAP,
    FRESHNESS_REPLAY,
    FRESHNESS_TTL_EXPIRED
} FreshnessResult;

typedef struct {
    bool initialized;
    uint16_t last_sequence;
} FreshnessState;

typedef struct {
    bool initialized;
    uint32_t accepted_at_ms;
} FreshnessLifetime;

void freshness_init(FreshnessState *state);
void freshness_reset(FreshnessState *state);

bool freshness_sequence_is_newer(uint16_t incoming, uint16_t last);
FreshnessResult freshness_check_sequence(const FreshnessState *state,
                                          uint16_t incoming);
void freshness_accept_sequence(FreshnessState *state, uint16_t sequence);

void freshness_lifetime_start(FreshnessLifetime *lifetime,
                              uint32_t accepted_at_ms);
uint32_t freshness_lifetime_elapsed(const FreshnessLifetime *lifetime,
                                    uint32_t now_ms);
bool freshness_lifetime_is_valid(const FreshnessLifetime *lifetime,
                                 uint32_t now_ms,
                                 uint32_t ttl_ms);

/*
 * Compatibility helper for callers that have already authenticated a frame.
 * It updates sequence state only after sequence and receiver-local age pass.
 */
FreshnessResult freshness_validate_and_accept(FreshnessState *state,
                                                uint16_t incoming,
                                                uint32_t age_ms,
                                                uint32_t ttl_ms);

bool freshness_result_is_accepted(FreshnessResult result);
const char *freshness_result_name(FreshnessResult result);

#ifdef __cplusplus
}
#endif

#endif
