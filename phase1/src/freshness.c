#include "freshness.h"

#define FRESHNESS_HALF_RANGE ((uint16_t)0x8000U)

void freshness_init(FreshnessState *state)
{
    if (state != 0) {
        state->initialized = false;
        state->last_sequence = 0U;
    }
}

void freshness_reset(FreshnessState *state)
{
    freshness_init(state);
}

bool freshness_sequence_is_newer(uint16_t incoming, uint16_t last)
{
    uint16_t delta = (uint16_t)(incoming - last);

    /* Unsigned subtraction intentionally wraps modulo 2^16. */
    return delta != 0U && delta < FRESHNESS_HALF_RANGE;
}

FreshnessResult freshness_check_sequence(const FreshnessState *state,
                                          uint16_t incoming)
{
    uint16_t delta;

    if (state == 0) {
        return FRESHNESS_INVALID_STATE;
    }
    if (!state->initialized) {
        return FRESHNESS_FIRST_ACCEPTED;
    }
    if (!freshness_sequence_is_newer(incoming, state->last_sequence)) {
        return FRESHNESS_REPLAY;
    }

    delta = (uint16_t)(incoming - state->last_sequence);
    return delta == 1U ? FRESHNESS_ACCEPTED : FRESHNESS_SEQUENCE_GAP;
}

void freshness_accept_sequence(FreshnessState *state, uint16_t sequence)
{
    if (state != 0) {
        state->initialized = true;
        state->last_sequence = sequence;
    }
}

void freshness_lifetime_start(FreshnessLifetime *lifetime,
                              uint32_t accepted_at_ms)
{
    if (lifetime != 0) {
        lifetime->initialized = true;
        lifetime->accepted_at_ms = accepted_at_ms;
    }
}

uint32_t freshness_lifetime_elapsed(const FreshnessLifetime *lifetime,
                                    uint32_t now_ms)
{
    if (lifetime == 0 || !lifetime->initialized) {
        return UINT32_MAX;
    }

    /* Unsigned subtraction supports a monotonic 32-bit timer wrap. */
    return (uint32_t)(now_ms - lifetime->accepted_at_ms);
}

bool freshness_lifetime_is_valid(const FreshnessLifetime *lifetime,
                                 uint32_t now_ms,
                                 uint32_t ttl_ms)
{
    if (lifetime == 0 || !lifetime->initialized) {
        return false;
    }
    return freshness_lifetime_elapsed(lifetime, now_ms) <= ttl_ms;
}

FreshnessResult freshness_validate_and_accept(FreshnessState *state,
                                                uint16_t incoming,
                                                uint32_t age_ms,
                                                uint32_t ttl_ms)
{
    FreshnessResult sequence_result;

    if (state == 0) {
        return FRESHNESS_INVALID_STATE;
    }

    sequence_result = freshness_check_sequence(state, incoming);
    if (sequence_result == FRESHNESS_REPLAY ||
        sequence_result == FRESHNESS_INVALID_STATE) {
        return sequence_result;
    }
    if (age_ms > ttl_ms) {
        return FRESHNESS_TTL_EXPIRED;
    }

    freshness_accept_sequence(state, incoming);
    return sequence_result;
}

bool freshness_result_is_accepted(FreshnessResult result)
{
    return result == FRESHNESS_FIRST_ACCEPTED ||
           result == FRESHNESS_ACCEPTED ||
           result == FRESHNESS_SEQUENCE_GAP;
}

const char *freshness_result_name(FreshnessResult result)
{
    switch (result) {
    case FRESHNESS_INVALID_STATE: return "INVALID_STATE";
    case FRESHNESS_FIRST_ACCEPTED: return "FIRST_ACCEPTED";
    case FRESHNESS_ACCEPTED: return "ACCEPTED";
    case FRESHNESS_SEQUENCE_GAP: return "SEQUENCE_GAP";
    case FRESHNESS_REPLAY: return "REPLAY";
    case FRESHNESS_TTL_EXPIRED: return "TTL_EXPIRED";
    default: return "INVALID_RESULT";
    }
}
