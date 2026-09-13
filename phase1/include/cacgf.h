#ifndef AEGIS_CACGF_H
#define AEGIS_CACGF_H

#if defined(__has_include)
# if __has_include(<stdint.h>)
#  include <stdint.h>
# else
   typedef unsigned int uint32_t;
# endif
#else
# include <stdint.h>
#endif

#if defined(__has_include)
# if __has_include(<stdbool.h>)
#  include <stdbool.h>
# endif
#else
# include <stdbool.h>
#endif

#ifndef __cplusplus
# ifndef __bool_true_false_are_defined
typedef unsigned char bool;
#define true 1
#define false 0
# endif
#endif

#if defined(__has_include)
# if __has_include("vehicle_context.h")
#  include "vehicle_context.h"
# else
   typedef struct VehicleContext VehicleContext;
# endif
#else
# include "vehicle_context.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CACGF_MAX_SPEED_KMH 5U

typedef enum {
    CACGF_DECISION_EXECUTE = 0,
    CACGF_DECISION_DELAY,
    CACGF_DECISION_REJECT,
    CACGF_DECISION_ESCALATE
} CacgfDecision;

typedef enum {
    CACGF_CRITICALITY_LOW = 0,
    CACGF_CRITICALITY_MEDIUM,
    CACGF_CRITICALITY_HIGH
} CacgfCriticality;

typedef enum {
    CACGF_RISK_LOW = 0,
    CACGF_RISK_MEDIUM,
    CACGF_RISK_HIGH,
    CACGF_RISK_UNKNOWN
} CacgfRiskLevel;

typedef enum {
    CACGF_ERR_NONE = 0,
    CACGF_SUCCESS_EXECUTE,
    CACGF_INFO_QUEUE_DELAYED,
    CACGF_ERR_INVALID_STATE,
    CACGF_ERR_AUTH_FAIL,
    CACGF_ERR_REPLAY,
    CACGF_ERR_TTL_EXPIRED,
    CACGF_ERR_STALE_CONTEXT,
    CACGF_ERR_QUEUE_FULL,
    CACGF_ERR_HIGH_RISK,
    CACGF_ERR_DETECTOR_UNKNOWN,
    CACGF_ERR_UNKNOWN_COMMAND
} CacgfReason;

typedef struct {
    uint32_t command_id;
    /* These fields model untrusted sender data and are never used for policy. */
    CacgfCriticality sender_criticality;
    bool sender_delayable;
    uint32_t sender_ttl_ms;
} CacgfCommand;

typedef struct {
    bool authenticated;
    bool fresh;
    bool ttl_valid;
} CacgfSecurityState;

typedef VehicleContext CacgfVehicleContext;

typedef struct {
    CacgfRiskLevel level;
    uint32_t timestamp_ms;
    bool detector_healthy;
} CacgfRiskState;

typedef struct {
    bool queue_full;
} CacgfQueueState;

typedef struct {
    CacgfDecision decision;
    CacgfReason reason;
} CacgfResult;

CacgfResult cacgf_evaluate(const CacgfCommand *command,
                           const CacgfSecurityState *security,
                           const CacgfVehicleContext *vehicle,
                           const CacgfRiskState *risk,
                           const CacgfQueueState *queue);

const char *cacgf_decision_name(CacgfDecision decision);
const char *cacgf_reason_name(CacgfReason reason);

#ifdef __cplusplus
}
#endif

#endif
