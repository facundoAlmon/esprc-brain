#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize Bluetooth HID host (Bluedroid + esp_hidh).
// Must be called after NVS flash init. state_ptr is VehicleState*.
void hid_gamepad_init(void *state_ptr);

// Enable or disable background scanning for HID gamepads.
void hid_gamepad_set_scanning(bool enabled);

#ifdef __cplusplus
}
#endif
