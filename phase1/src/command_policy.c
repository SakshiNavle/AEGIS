#include "command_policy.h"

#include <stddef.h>

static const CommandPolicy command_policies[] = {
    { 0x100U, CACGF_CRITICALITY_LOW, true, 200U },
    { 0x150U, CACGF_CRITICALITY_LOW, true, 500U },
    { 0x200U, CACGF_CRITICALITY_HIGH, false, 200U },
    { 0x250U, CACGF_CRITICALITY_HIGH, false, 200U }
};

bool command_policy_resolve(uint32_t command_id, CommandPolicy *policy)
{
    size_t index;

    if (policy == 0) {
        return false;
    }

    for (index = 0U;
         index < sizeof(command_policies) / sizeof(command_policies[0]);
         ++index) {
        if (command_policies[index].command_id == command_id) {
            *policy = command_policies[index];
            return true;
        }
    }

    return false;
}
