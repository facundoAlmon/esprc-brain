// Custom bluepad32 platform — replaces bluepad32_arduino's arduino_platform.c.
// Bridges the uni_platform_t callbacks into our GamepadData struct in VehicleState.

#include "bp32_platform.h"
#include "gamepad_types.h"
#include "state.h"
#include "esp_log.h"
#include "uni.h"
#include "bt/uni_bt.h"
#include "controller/uni_controller.h"
#include "controller/uni_gamepad.h"

static const char* TAG = "BP32Platform";

static VehicleState* s_state = nullptr;
// Map uni_hid_device_t* → slot index in s_state->myGamepads
static uni_hid_device_t* s_device_map[MAX_GAMEPADS] = {};

void bp32_platform_set_vehicle_state(void* state) {
    s_state = (VehicleState*)state;
}

void bp32_set_scanning(bool enabled) {
    if (enabled) {
        uni_bt_start_scanning_and_autoconnect_safe();
    } else {
        uni_bt_stop_scanning_safe();
    }
}

// ---- Platform callbacks ----

static void bp32_init(int argc, const char** argv) {
    (void)argc; (void)argv;
}

static void bp32_on_init_complete(void) {
    ESP_LOGI(TAG, "Bluepad32 init complete");
    if (s_state && s_state->enableScan) {
        uni_bt_start_scanning_and_autoconnect_safe();
    }
}

static void bp32_on_device_connected(uni_hid_device_t* d) {
    // Slot will be confirmed in on_device_ready.
}

static uni_error_t bp32_on_device_ready(uni_hid_device_t* d) {
    if (!s_state) return UNI_ERROR_NO_SLOTS;

    for (int i = 0; i < MAX_GAMEPADS; i++) {
        if (s_device_map[i] == nullptr) {
            s_device_map[i] = d;
            GamepadData* gp = &s_state->myGamepads[i];
            memset(gp, 0, sizeof(GamepadData));
            gp->connected = true;
            strncpy(gp->model_name, d->name[0] ? d->name : "Unknown", sizeof(gp->model_name) - 1);
            gp->vendor_id  = d->vendor_id;
            gp->product_id = d->product_id;
            ESP_LOGI(TAG, "Gamepad connected → slot %d: %s (VID=0x%04x PID=0x%04x)",
                     i, gp->model_name, gp->vendor_id, gp->product_id);
            // Stop scanning once a gamepad is connected.
            uni_bt_stop_scanning_safe();
            return UNI_ERROR_SUCCESS;
        }
    }
    ESP_LOGW(TAG, "Gamepad connected but no free slots");
    return UNI_ERROR_NO_SLOTS;
}

static void bp32_on_device_disconnected(uni_hid_device_t* d) {
    if (!s_state) return;

    for (int i = 0; i < MAX_GAMEPADS; i++) {
        if (s_device_map[i] == d) {
            ESP_LOGI(TAG, "Gamepad disconnected from slot %d", i);
            s_device_map[i] = nullptr;
            memset(&s_state->myGamepads[i], 0, sizeof(GamepadData));
            s_state->myGamepads[i].connected = false;
            break;
        }
    }
}

static void bp32_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    if (!s_state || ctl->klass != UNI_CONTROLLER_CLASS_GAMEPAD) return;

    for (int i = 0; i < MAX_GAMEPADS; i++) {
        if (s_device_map[i] == d) {
            GamepadData* gp = &s_state->myGamepads[i];
            const uni_gamepad_t* src = &ctl->gamepad;
            gp->axis_x   = (int16_t)src->axis_x;
            gp->axis_y   = (int16_t)src->axis_y;
            gp->throttle = (int16_t)src->throttle;
            gp->brake    = (int16_t)src->brake;
            gp->buttons  = src->buttons;
            gp->dpad     = src->dpad;
            break;
        }
    }
}

static const uni_property_t* bp32_get_property(uni_property_idx_t idx) {
    (void)idx;
    return nullptr;
}

static void bp32_on_oob_event(uni_platform_oob_event_t event, void* data) {
    (void)data;
    if (event == UNI_PLATFORM_OOB_BLUETOOTH_ENABLED) {
        ESP_LOGI(TAG, "Bluetooth scanning: %s", uni_bt_is_scanning() ? "ON" : "OFF");
    }
}

// ---- Platform struct ----

static struct uni_platform s_platform = {
    .name                = "ESP-RC custom",
    .init                = bp32_init,
    .on_init_complete    = bp32_on_init_complete,
    .on_device_discovered = nullptr,
    .on_device_connected = bp32_on_device_connected,
    .on_device_disconnected = bp32_on_device_disconnected,
    .on_device_ready     = bp32_on_device_ready,
    .on_gamepad_data     = nullptr,
    .on_controller_data  = bp32_on_controller_data,
    .get_property        = bp32_get_property,
    .on_oob_event        = bp32_on_oob_event,
    .device_dump         = nullptr,
    .register_console_cmds = nullptr,
};

struct uni_platform* get_bp32_platform(void) {
    return &s_platform;
}
