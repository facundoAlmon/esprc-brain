#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "platform/uni_platform.h"

// Returns the custom platform struct for uni_platform_set_custom().
struct uni_platform* get_bp32_platform(void);

// Set the VehicleState pointer. Must be called before uni_init().
// Declared as void* to keep this header pure-C.
void bp32_platform_set_vehicle_state(void* state);

// Enable or disable scanning for new gamepads.
void bp32_set_scanning(bool enabled);

#ifdef __cplusplus
}
#endif
