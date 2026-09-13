#ifndef AEGIS_GUARDCAN_H
#define AEGIS_GUARDCAN_H

#include "cacgf.h"
#include "freshness.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GUARDCAN_MAX_PAYLOAD 8U
#define GUARDCAN_WINDOW_MS 1000U
#define GUARDCAN_ELEVATED_MESSAGES_PER_WINDOW 20U
#define GUARDCAN_HIGH_MESSAGES_PER_WINDOW 50U
#define GUARDCAN_BURST_INTERVAL_MS 10U

typedef enum {
    GUARDCAN_RISK_LOW = 0,
    GUARDCAN_RISK_MEDIUM,
    GUARDCAN_RISK_HIGH,
    GUARDCAN_RISK_UNKNOWN
} GuardcanRiskLevel;

typedef enum {
    GUARDCAN_HEALTH_OK = 0,
    GUARDCAN_HEALTH_DEGRADED,
    GUARDCAN_HEALTH_FAILED
} GuardcanDetectorHealth;

typedef struct {
    uint16_t can_id;
    uint8_t dlc;
    uint8_t payload[GUARDCAN_MAX_PAYLOAD];
    uint32_t timestamp_ms;
} GuardcanFrame;

typedef struct {
    GuardcanRiskLevel risk;
    GuardcanDetectorHealth health;
    uint16_t evidence_score;
    uint32_t last_update_ms;
    uint32_t risk_age_ms;
} GuardcanSnapshot;

typedef struct {
    bool initialized;
    uint32_t window_start_ms;
    uint16_t window_count;
    uint32_t last_frame_ms;
    uint16_t last_sequence;
    bool sequence_initialized;
    uint16_t evidence_score;
    uint32_t last_update_ms;
    GuardcanRiskLevel risk;
    GuardcanDetectorHealth health;
} GuardcanMonitor;

void guardcan_init(GuardcanMonitor *monitor);
bool guardcan_observe_frame(GuardcanMonitor *monitor,
                            const GuardcanFrame *frame);
bool guardcan_snapshot(const GuardcanMonitor *monitor,
                       uint32_t now_ms,
                       GuardcanSnapshot *snapshot);
void guardcan_recover(GuardcanMonitor *monitor, uint32_t now_ms);
GuardcanRiskLevel guardcan_risk(const GuardcanMonitor *monitor);
GuardcanDetectorHealth guardcan_health(const GuardcanMonitor *monitor);
CacgfRiskLevel guardcan_to_cacgf_risk(GuardcanRiskLevel risk);
const char *guardcan_risk_name(GuardcanRiskLevel risk);
const char *guardcan_health_name(GuardcanDetectorHealth health);

#ifdef __cplusplus
}
#endif

#endif
