#ifndef AEGIS_COMMAND_POLICY_H
#define AEGIS_COMMAND_POLICY_H

#include "cacgf.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t command_id;
    CacgfCriticality criticality;
    bool delayable;
    uint32_t ttl_ms;
} CommandPolicy;

bool command_policy_resolve(uint32_t command_id, CommandPolicy *policy);

#ifdef __cplusplus
}
#endif

#endif
