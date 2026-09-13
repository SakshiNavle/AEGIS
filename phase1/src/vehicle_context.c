#include "vehicle_context.h"

static void invalidate_context(VehicleContextStore *store,
                               VehicleContextValidity validity)
{
    store->context.validity = validity;
}

static bool valid_gear(VehicleGear gear)
{
    return gear >= VEHICLE_GEAR_PARK && gear <= VEHICLE_GEAR_REVERSE;
}

void vehicle_context_init(VehicleContextStore *store)
{
    if (store != 0) {
        store->context.speed_kmh = 0U;
        store->context.gear = VEHICLE_GEAR_UNKNOWN;
        store->context.brake_pressed = false;
        store->context.telemetry_sequence = 0U;
        store->context.telemetry_age_ms = 0U;
        store->context.validity = CONTEXT_UNKNOWN;
        freshness_init(&store->telemetry_freshness);
    }
}

VehicleContextValidity vehicle_context_update(VehicleContextStore *store,
                                              const VehicleTelemetry *telemetry)
{
    FreshnessResult sequence_result;

    if (store == 0 || telemetry == 0) {
        if (store != 0) {
            invalidate_context(store, CONTEXT_INVALID);
        }
        return CONTEXT_INVALID;
    }
    if (!valid_gear(telemetry->gear)) {
        invalidate_context(store, CONTEXT_INVALID);
        return CONTEXT_INVALID;
    }

    sequence_result = freshness_check_sequence(
        &store->telemetry_freshness, telemetry->telemetry_sequence);
    if (sequence_result == FRESHNESS_REPLAY ||
        sequence_result == FRESHNESS_INVALID_STATE) {
        invalidate_context(store, CONTEXT_STALE);
        return CONTEXT_STALE;
    }
    if (telemetry->telemetry_age_ms > VEHICLE_CONTEXT_TELEMETRY_TTL_MS) {
        invalidate_context(store, CONTEXT_STALE);
        return CONTEXT_STALE;
    }

    store->context.speed_kmh = telemetry->speed_kmh;
    store->context.gear = telemetry->gear;
    store->context.brake_pressed = telemetry->brake_pressed;
    store->context.telemetry_sequence = telemetry->telemetry_sequence;
    store->context.telemetry_age_ms = telemetry->telemetry_age_ms;
    store->context.validity = CONTEXT_VALID;
    freshness_accept_sequence(&store->telemetry_freshness,
                               telemetry->telemetry_sequence);
    return CONTEXT_VALID;
}

const VehicleContext *vehicle_context_current(
    const VehicleContextStore *store)
{
    return store == 0 ? 0 : &store->context;
}

bool vehicle_context_is_valid(const VehicleContext *context)
{
    return context != 0 && context->validity == CONTEXT_VALID &&
           valid_gear(context->gear);
}

const char *vehicle_context_validity_name(VehicleContextValidity validity)
{
    switch (validity) {
    case CONTEXT_UNKNOWN: return "CONTEXT_UNKNOWN";
    case CONTEXT_INVALID: return "CONTEXT_INVALID";
    case CONTEXT_STALE: return "CONTEXT_STALE";
    case CONTEXT_VALID: return "CONTEXT_VALID";
    default: return "INVALID_CONTEXT_STATE";
    }
}