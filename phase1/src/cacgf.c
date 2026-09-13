#include "cacgf.h"
#include "command_policy.h"

static CacgfResult result(CacgfDecision decision, CacgfReason reason)
{
    CacgfResult value = { decision, reason };
    return value;
}

static bool valid_risk(CacgfRiskLevel risk)
{
    return risk >= CACGF_RISK_LOW && risk <= CACGF_RISK_UNKNOWN;
}

CacgfResult cacgf_evaluate(const CacgfCommand *command,
                           const CacgfSecurityState *security,
                           const CacgfVehicleContext *vehicle,
                           const CacgfRiskState *risk,
                           const CacgfQueueState *queue)
{
    CommandPolicy policy;

    if (command == 0 || security == 0 || vehicle == 0 || risk == 0 ||
        queue == 0 || !valid_risk(risk->level)) {
        return result(CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    }

    if (vehicle->validity != CONTEXT_VALID) {
        return result(CACGF_DECISION_REJECT, CACGF_ERR_STALE_CONTEXT);
    }
    if (!vehicle_context_is_valid(vehicle)) {
        return result(CACGF_DECISION_REJECT, CACGF_ERR_INVALID_STATE);
    }

    /* Authentication and freshness always take precedence over policy checks. */
    if (!security->authenticated) {
        return result(CACGF_DECISION_REJECT, CACGF_ERR_AUTH_FAIL);
    }
    if (!security->fresh) {
        return result(CACGF_DECISION_REJECT, CACGF_ERR_REPLAY);
    }
    if (!security->ttl_valid) {
        return result(CACGF_DECISION_REJECT, CACGF_ERR_TTL_EXPIRED);
    }
    if (!command_policy_resolve(command->command_id, &policy)) {
        return result(CACGF_DECISION_REJECT, CACGF_ERR_UNKNOWN_COMMAND);
    }

    if (policy.delayable && policy.criticality == CACGF_CRITICALITY_LOW &&
        vehicle->speed_kmh > CACGF_MAX_SPEED_KMH) {
        if (queue->queue_full) {
            return result(CACGF_DECISION_REJECT, CACGF_ERR_QUEUE_FULL);
        }
        return result(CACGF_DECISION_DELAY, CACGF_INFO_QUEUE_DELAYED);
    }

    if (risk->level == CACGF_RISK_HIGH &&
        policy.criticality == CACGF_CRITICALITY_HIGH) {
        return result(CACGF_DECISION_ESCALATE, CACGF_ERR_HIGH_RISK);
    }

    if (risk->level == CACGF_RISK_UNKNOWN &&
        policy.criticality != CACGF_CRITICALITY_LOW) {
        return result(CACGF_DECISION_REJECT, CACGF_ERR_DETECTOR_UNKNOWN);
    }

    return result(CACGF_DECISION_EXECUTE, CACGF_SUCCESS_EXECUTE);
}

const char *cacgf_decision_name(CacgfDecision decision)
{
    switch (decision) {
    case CACGF_DECISION_EXECUTE: return "EXECUTE";
    case CACGF_DECISION_DELAY: return "DELAY";
    case CACGF_DECISION_REJECT: return "REJECT";
    case CACGF_DECISION_ESCALATE: return "ESCALATE";
    default: return "INVALID_DECISION";
    }
}

const char *cacgf_reason_name(CacgfReason reason)
{
    switch (reason) {
    case CACGF_ERR_NONE: return "ERR_NONE";
    case CACGF_SUCCESS_EXECUTE: return "SUCCESS_EXECUTE";
    case CACGF_INFO_QUEUE_DELAYED: return "INFO_QUEUE_DELAYED";
    case CACGF_ERR_INVALID_STATE: return "ERR_INVALID_STATE";
    case CACGF_ERR_AUTH_FAIL: return "ERR_AUTH_FAIL";
    case CACGF_ERR_REPLAY: return "ERR_REPLAY";
    case CACGF_ERR_TTL_EXPIRED: return "ERR_TTL_EXPIRED";
    case CACGF_ERR_STALE_CONTEXT: return "ERR_STALE_CONTEXT";
    case CACGF_ERR_QUEUE_FULL: return "ERR_QUEUE_FULL";
    case CACGF_ERR_HIGH_RISK: return "ERR_HIGH_RISK";
    case CACGF_ERR_DETECTOR_UNKNOWN: return "ERR_DETECTOR_UNKNOWN";
    case CACGF_ERR_UNKNOWN_COMMAND: return "ERR_UNKNOWN_COMMAND";
    default: return "INVALID_REASON";
    }
}
