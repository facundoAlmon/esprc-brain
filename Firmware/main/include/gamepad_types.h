#pragma once
#include <stdbool.h>
#include <stdint.h>

#define MAX_GAMEPADS 4

// Button bitmasks — parsed from HID reports in hid_gamepad.cpp.
#define GAMEPAD_BTN_A          (1u << 0)
#define GAMEPAD_BTN_B          (1u << 1)
#define GAMEPAD_BTN_X          (1u << 2)
#define GAMEPAD_BTN_Y          (1u << 3)
#define GAMEPAD_BTN_L1         (1u << 4)
#define GAMEPAD_BTN_R1         (1u << 5)
#define GAMEPAD_BTN_TRIGGER_L  (1u << 6)
#define GAMEPAD_BTN_TRIGGER_R  (1u << 7)
#define GAMEPAD_BTN_THUMB_L    (1u << 8)
#define GAMEPAD_BTN_THUMB_R    (1u << 9)
#define GAMEPAD_BTN_SELECT     (1u << 10)  // View/Back/Select según el mando

typedef struct {
    bool     connected;
    int16_t  axis_x;     // -512 to 511 (left stick X)
    int16_t  axis_y;     // -512 to 511 (left stick Y)
    int16_t  axis_rx;    // -512 to 511 (right stick X → camera pan)
    int16_t  axis_ry;    // -512 to 511 (right stick Y → camera tilt)
    int16_t  throttle;   // 0-1023 (right trigger)
    int16_t  brake;      // 0-1023 (left trigger)
    uint32_t buttons;    // bitmask, see GAMEPAD_BTN_* macros
    uint8_t  dpad;
    char     model_name[32];
    uint16_t vendor_id;
    uint16_t product_id;
} GamepadData;
