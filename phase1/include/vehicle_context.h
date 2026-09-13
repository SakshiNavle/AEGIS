#ifndef AEGIS_VEHICLE_CONTEXT_H
#define AEGIS_VEHICLE_CONTEXT_H

#include "freshness.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VEHICLE_CONTEXT_TELEMETRY_TTL_MS 50U

typedef enum {
    VEHICLE_GEAR_UNKNOWN = 0,
    VEHICLE_GEAR_PARK,
    VEHICLE_GEAR_NEUTRAL,
    VEHICLE_GEAR_DRIVE,
    VEHICLE_GEAR_REVERSE
} VehicleGear;

typedef enum {
    CONTEXT_UNKNOWN = 0,
    CONTEXT_INVALID,
    CONTEXT_STALE,
    CONTEXT_VALID
} VehicleContextValidity;

typedef struct {
    uint16_t speed_kmh;
    VehicleGear gear;
    bool brake_pressed;
    uint16_t telemetry_sequence;
    uint32_t telemetry_age_ms;
    VehicleContextValidity validity;
} VehicleContext;

typedef struct {
    VehicleContext context;
    FreshnessState telemetry_freshness;
} VehicleContextStore;

typedef struct {
    uint16_t speed_kmh;
    VehicleGear gear;
    bool brake_pressed;
    uint16_t telemetry_sequence;
    uint32_t telemetry_age_ms;
} VehicleTelemetry;

void vehicle_context_init(VehicleContextStore *store);
VehicleContextValidity vehicle_context_update(VehicleContextStore *store,
                                              const VehicleTelemetry *telemetry);
const VehicleContext *vehicle_context_current(
    const VehicleContextStore *store);
bool vehicle_context_is_valid(const VehicleContext *context);
const char *vehicle_context_validity_name(VehicleContextValidity validity);

#ifdef __cplusplus
}
#endif

#endif