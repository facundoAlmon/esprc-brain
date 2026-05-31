#pragma once

#include "state.h"
#include "esp_wifi_types.h"

// Call once after esp_wifi_start().
void espnow_init(VehicleState* state, wifi_interface_t iface);
