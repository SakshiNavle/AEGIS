#include "guardcan.h"

#include <string.h>

typedef struct {
    uint16_t can_id;
    uint8_t expected_dlc;
} GuardcanKnownId;

static const GuardcanKnownId known_ids[] = {
    { 0x060U, 4U },
    { 0x100U, 2U },
    { 0x150U, 2U },
    { 0x200U, 2U },
    { 0x250U, 2U },
    { 0x700U, 2U }
};

static bool elapsed_at_least(uint32_t now, uint32_t then, uint32_t duration)
{
    return (uint32_t)(now - then) >= duration;
}

static bool find_id(uint16_t can_id, uint8_t *expected_dlc)
{
    size_t index;

    for (index = 0U; index < sizeof(known_ids) / sizeof(known_ids[0]);
         ++index) {
        if (known_ids[index].can_id == can_id) {
            *expected_dlc = known_ids[index].expected_dlc;
            return true;
        }
    }
    return false;
}

static GuardcanRiskLevel score_to_risk(uint16_t score)
{
    if (score == 0U) {
        return GUARDCAN_RISK_LOW;
    }
    if (score < 3U) {
        return GUARDCAN_RISK_MEDIUM;
    }
    return GUARDCAN_RISK_HIGH;
}

static void add_evidence(GuardcanMonitor *monitor, uint16_t amount)
{
    if (UINT16_MAX - monitor->evidence_score < amount) {
        monitor->evidence_score = UINT16_MAX;
    } else {
        monitor->evidence_score = (uint16_t)(monitor->evidence_score + amount);
    }
}

static void refresh_risk(GuardcanMonitor *monitor)
{
    if (monitor->health != GUARDCAN_HEALTH_OK) {
        monitor->risk = GUARDCAN_RISK_UNKNOWN;
    } else {
        monitor->risk = score_to_risk(monitor->evidence_score);
    }
}

void guardcan_init(GuardcanMonitor *monitor)
{
    if (monitor != 0) {
        memset(monitor, 0, sizeof(*monitor));
        monitor->risk = GUARDCAN_RISK_UNKNOWN;
        monitor->health = GUARDCAN_HEALTH_OK;
    }
}

bool guardcan_observe_frame(GuardcanMonitor *monitor,
                            const GuardcanFrame *frame)
{
    uint8_t expected_dlc;
    FreshnessResult sequence_result;
    uint16_t sequence;
    bool unexpected;
    bool malformed;
    bool burst;

    if (monitor == 0 || frame == 0 || frame->can_id > 0x7FFU ||
        frame->dlc > GUARDCAN_MAX_PAYLOAD) {
        if (monitor != 0) {
            monitor->health = GUARDCAN_HEALTH_DEGRADED;
            monitor->risk = GUARDCAN_RISK_UNKNOWN;
        }
        return false;
    }

    if (!monitor->initialized) {
        monitor->initialized = true;
        monitor->window_start_ms = frame->timestamp_ms;
    }
    if (elapsed_at_least(frame->timestamp_ms, monitor->window_start_ms,
                         GUARDCAN_WINDOW_MS)) {
        monitor->window_start_ms = frame->timestamp_ms;
        monitor->window_count = 0U;
        if (monitor->evidence_score > 0U) {
            --monitor->evidence_score;
        }
    }
    if (monitor->window_count < UINT16_MAX) {
        ++monitor->window_count;
    }

    unexpected = !find_id(frame->can_id, &expected_dlc);
    malformed = !unexpected && frame->dlc != expected_dlc;
    burst = monitor->last_frame_ms != 0U &&
            (uint32_t)(frame->timestamp_ms - monitor->last_frame_ms) <
            GUARDCAN_BURST_INTERVAL_MS;

    if (malformed) {
        monitor->health = GUARDCAN_HEALTH_DEGRADED;
        monitor->risk = GUARDCAN_RISK_UNKNOWN;
        monitor->last_update_ms = frame->timestamp_ms;
        return false;
    }

    if (unexpected) {
        add_evidence(monitor, 2U);
    }
    if (monitor->window_count > GUARDCAN_HIGH_MESSAGES_PER_WINDOW) {
        add_evidence(monitor, 3U);
    } else if (monitor->window_count > GUARDCAN_ELEVATED_MESSAGES_PER_WINDOW) {
        add_evidence(monitor, 1U);
    }
    if (burst) {
        add_evidence(monitor, 1U);
    }

    if (frame->dlc >= 2U) {
        sequence = (uint16_t)frame->payload[0] |
                   (uint16_t)((uint16_t)frame->payload[1] << 8);
        if (monitor->sequence_initialized) {
            sequence_result = freshness_check_sequence(
                &(FreshnessState){ true, monitor->last_sequence }, sequence);
            if (sequence_result == FRESHNESS_REPLAY) {
                add_evidence(monitor, 2U);
            } else if (sequence_result == FRESHNESS_SEQUENCE_GAP) {
                add_evidence(monitor, 1U);
            }
            if (sequence_result != FRESHNESS_REPLAY) {
                monitor->last_sequence = sequence;
            }
        } else {
            monitor->last_sequence = sequence;
        }
        if (!monitor->sequence_initialized ||
            sequence_result != FRESHNESS_REPLAY) {
            monitor->sequence_initialized = true;
        }
    }

    monitor->last_frame_ms = frame->timestamp_ms;
    monitor->last_update_ms = frame->timestamp_ms;
    refresh_risk(monitor);
    return true;
}

void guardcan_recover(GuardcanMonitor *monitor, uint32_t now_ms)
{
    if (monitor == 0 || monitor->health == GUARDCAN_HEALTH_FAILED ||
        !monitor->initialized) {
        return;
    }
    if (elapsed_at_least(now_ms, monitor->last_update_ms, GUARDCAN_WINDOW_MS)) {
        monitor->evidence_score = 0U;
        monitor->health = GUARDCAN_HEALTH_OK;
        monitor->last_update_ms = now_ms;
        refresh_risk(monitor);
    }
}

bool guardcan_snapshot(const GuardcanMonitor *monitor,
                       uint32_t now_ms,
                       GuardcanSnapshot *snapshot)
{
    if (monitor == 0 || snapshot == 0 || !monitor->initialized) {
        return false;
    }
    snapshot->risk = monitor->risk;
    snapshot->health = monitor->health;
    snapshot->evidence_score = monitor->evidence_score;
    snapshot->last_update_ms = monitor->last_update_ms;
    snapshot->risk_age_ms = (uint32_t)(now_ms - monitor->last_update_ms);
    return true;
}

GuardcanRiskLevel guardcan_risk(const GuardcanMonitor *monitor)
{
    return monitor == 0 ? GUARDCAN_RISK_UNKNOWN : monitor->risk;
}

GuardcanDetectorHealth guardcan_health(const GuardcanMonitor *monitor)
{
    return monitor == 0 ? GUARDCAN_HEALTH_FAILED : monitor->health;
}

CacgfRiskLevel guardcan_to_cacgf_risk(GuardcanRiskLevel risk)
{
    switch (risk) {
    case GUARDCAN_RISK_LOW: return CACGF_RISK_LOW;
    case GUARDCAN_RISK_MEDIUM: return CACGF_RISK_MEDIUM;
    case GUARDCAN_RISK_HIGH: return CACGF_RISK_HIGH;
    default: return CACGF_RISK_UNKNOWN;
    }
}

const char *guardcan_risk_name(GuardcanRiskLevel risk)
{
    switch (risk) {
    case GUARDCAN_RISK_LOW: return "LOW";
    case GUARDCAN_RISK_MEDIUM: return "MEDIUM";
    case GUARDCAN_RISK_HIGH: return "HIGH";
    case GUARDCAN_RISK_UNKNOWN: return "UNKNOWN";
    default: return "INVALID_RISK";
    }
}

const char *guardcan_health_name(GuardcanDetectorHealth health)
{
    switch (health) {
    case GUARDCAN_HEALTH_OK: return "OK";
    case GUARDCAN_HEALTH_DEGRADED: return "DEGRADED";
    case GUARDCAN_HEALTH_FAILED: return "FAILED";
    default: return "INVALID_HEALTH";
    }
}
