#include "hid_gamepad.h"
#include "gamepad_types.h"
#include "state.h"
#include "esp_hid_gap.h"
#include "esp_hidh.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "HidGamepad";

// VID for all Microsoft Xbox controllers
#define XBOX_VID 0x045E

static VehicleState *s_state = nullptr;
static bool s_scanning = false;
static TaskHandle_t s_scan_task = nullptr;
static esp_hidh_dev_t *s_dev_map[MAX_GAMEPADS] = {};

// ---- Xbox Series X/S BLE HID report parser ----
// Report ID 0x01, 16-byte payload (report ID stripped by esp_hidh):
//   Byte 0 bits [3:0]  Hat switch  0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW 8=Center
//   Byte 0 bits [7:4]  Buttons A, B, View, X
//   Byte 1 bits [7:0]  Buttons Y, Menu, LB, RB, Guide, Share, LS, RS
//   Bytes  2-3   Left X  int16
//   Bytes  4-5   Left Y  int16
//   Bytes  6-7   Right X int16
//   Bytes  8-9   Right Y int16
//   Bytes 10-11  Left  Trigger uint16 0-1023
//   Bytes 12-13  Right Trigger uint16 0-1023
//   Bytes 14-15  Padding
static void parse_xbox(GamepadData *gp, const uint8_t *d, uint16_t len)
{
    if (len < 14) return;

    gp->dpad = d[0] & 0x0F;

    uint16_t btn = (uint16_t)(d[0] >> 4) | ((uint16_t)d[1] << 4);
    gp->buttons = 0;
    if (btn & (1u << 0))  gp->buttons |= GAMEPAD_BTN_A;
    if (btn & (1u << 1))  gp->buttons |= GAMEPAD_BTN_B;
    // bit 2 = View (Back)
    if (btn & (1u << 3))  gp->buttons |= GAMEPAD_BTN_X;
    if (btn & (1u << 4))  gp->buttons |= GAMEPAD_BTN_Y;
    // bit 5 = Menu (Start)
    if (btn & (1u << 6))  gp->buttons |= GAMEPAD_BTN_L1;   // LB
    if (btn & (1u << 7))  gp->buttons |= GAMEPAD_BTN_R1;   // RB
    // bit 8 = Guide
    // bit 9 = Share
    if (btn & (1u << 10)) gp->buttons |= GAMEPAD_BTN_THUMB_L;  // LS
    if (btn & (1u << 11)) gp->buttons |= GAMEPAD_BTN_THUMB_R;  // RS

    int16_t raw_lx = (int16_t)((uint16_t)d[2] | ((uint16_t)d[3] << 8));
    int16_t raw_ly = (int16_t)((uint16_t)d[4] | ((uint16_t)d[5] << 8));
    gp->axis_x = (int16_t)((int32_t)raw_lx * 512 / 32767);
    gp->axis_y = (int16_t)((int32_t)raw_ly * 512 / 32767);

    gp->brake    = (int16_t)((uint16_t)d[10] | ((uint16_t)d[11] << 8));  // Left  trigger
    gp->throttle = (int16_t)((uint16_t)d[12] | ((uint16_t)d[13] << 8));  // Right trigger
}

// ---- Generic HID gamepad parser ----
// Handles the common 8-byte compact layout:
//   Byte 0  Left X  0-255, center 128
//   Byte 1  Left Y  0-255, center 128
//   Byte 2  Right X
//   Byte 3  Right Y
//   Byte 4  Left  Trigger 0-255
//   Byte 5  Right Trigger 0-255
//   Bytes 6-7  Buttons bitmask (A=b0, B=b1, X=b2, Y=b3, LB=b4, RB=b5, LS=b6, RS=b7)
static void parse_generic(GamepadData *gp, const uint8_t *d, uint16_t len)
{
    if (len < 8) return;

    gp->axis_x = (int16_t)((int32_t)((int16_t)d[0] - 128) * 512 / 127);
    gp->axis_y = (int16_t)((int32_t)((int16_t)d[1] - 128) * 512 / 127);
    gp->brake    = (int16_t)((uint16_t)d[4] * 4);  // 0-255 → 0-1020
    gp->throttle = (int16_t)((uint16_t)d[5] * 4);

    uint16_t btn = (uint16_t)d[6] | ((uint16_t)d[7] << 8);
    gp->buttons = 0;
    if (btn & (1u << 0)) gp->buttons |= GAMEPAD_BTN_A;
    if (btn & (1u << 1)) gp->buttons |= GAMEPAD_BTN_B;
    if (btn & (1u << 2)) gp->buttons |= GAMEPAD_BTN_X;
    if (btn & (1u << 3)) gp->buttons |= GAMEPAD_BTN_Y;
    if (btn & (1u << 4)) gp->buttons |= GAMEPAD_BTN_L1;
    if (btn & (1u << 5)) gp->buttons |= GAMEPAD_BTN_R1;
    if (btn & (1u << 6)) gp->buttons |= GAMEPAD_BTN_THUMB_L;
    if (btn & (1u << 7)) gp->buttons |= GAMEPAD_BTN_THUMB_R;
}

// ---- Device slot helpers ----
static int find_free_slot(void)
{
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        if (s_dev_map[i] == nullptr) return i;
    }
    return -1;
}

static int find_dev_slot(esp_hidh_dev_t *dev)
{
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        if (s_dev_map[i] == dev) return i;
    }
    return -1;
}

// Forward declaration so scan_task can call hid_gamepad_set_scanning indirectly
static void start_scan_task(void);

// ---- HIDH event callback ----
static void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDH_OPEN_EVENT: {
        if (param->open.status != ESP_OK) {
            ESP_LOGW(TAG, "HIDH open failed (status=%d)", param->open.status);
            if (s_scanning && s_scan_task == nullptr) {
                start_scan_task();
            }
            break;
        }
        esp_hidh_dev_t *dev = param->open.dev;
        int slot = find_free_slot();
        if (slot < 0) {
            ESP_LOGW(TAG, "No free gamepad slots");
            esp_hidh_dev_close(dev);
            break;
        }
        s_dev_map[slot] = dev;
        if (s_state) {
            GamepadData *gp = &s_state->myGamepads[slot];
            memset(gp, 0, sizeof(GamepadData));
            gp->connected  = true;
            gp->vendor_id  = esp_hidh_dev_vendor_id_get(dev);
            gp->product_id = esp_hidh_dev_product_id_get(dev);
            const char *name = esp_hidh_dev_name_get(dev);
            strncpy(gp->model_name, name ? name : "Unknown", sizeof(gp->model_name) - 1);
            ESP_LOGI(TAG, "Gamepad slot %d: %s (VID=0x%04x PID=0x%04x)",
                     slot, gp->model_name, gp->vendor_id, gp->product_id);
        }
        break;
    }

    case ESP_HIDH_CLOSE_EVENT: {
        esp_hidh_dev_t *dev = param->close.dev;
        int slot = find_dev_slot(dev);
        if (slot >= 0) {
            ESP_LOGI(TAG, "Gamepad slot %d disconnected", slot);
            s_dev_map[slot] = nullptr;
            if (s_state) {
                memset(&s_state->myGamepads[slot], 0, sizeof(GamepadData));
            }
        }
        esp_hidh_dev_free(dev);  // Required: frees internal device memory
        if (s_scanning && s_scan_task == nullptr) {
            start_scan_task();
        }
        break;
    }

    case ESP_HIDH_INPUT_EVENT: {
        if (!s_state) break;
        int slot = find_dev_slot(param->input.dev);
        if (slot < 0) break;
        GamepadData *gp = &s_state->myGamepads[slot];
        if (gp->vendor_id == XBOX_VID && param->input.report_id == 0x01) {
            parse_xbox(gp, param->input.data, param->input.length);
        } else {
            parse_generic(gp, param->input.data, param->input.length);
        }
        break;
    }

    default:
        break;
    }
}

// ---- Scan task ----
static void scan_task(void *pvParams)
{
    while (s_scanning) {
        size_t num_results = 0;
        esp_hid_scan_result_t *results = nullptr;
        ESP_LOGI(TAG, "Scanning for HID gamepads (5s BLE + 5s BT)...");
        esp_hid_scan(5, &num_results, &results);
        ESP_LOGI(TAG, "Scan complete: %d device(s) found", (int)num_results);

        bool connecting = false;
        if (num_results > 0 && s_scanning) {
            esp_hid_scan_result_t *r = results;
            esp_hid_scan_result_t *best = nullptr;
            while (r) {
                if (r->usage == ESP_HID_USAGE_GAMEPAD || r->usage == ESP_HID_USAGE_JOYSTICK) {
                    best = r;
                    break;
                }
                r = r->next;
            }
            if (best == nullptr) {
                best = results;  // Fallback: first HID device
            }
            uint8_t addr_type = (best->transport == ESP_HID_TRANSPORT_BLE) ? best->ble.addr_type : 0;
            ESP_LOGI(TAG, "Connecting to: %s [%s]",
                     best->name ? best->name : "Unknown",
                     best->transport == ESP_HID_TRANSPORT_BLE ? "BLE" : "BT");
            esp_hidh_dev_open(best->bda, best->transport, addr_type);
            connecting = true;
        }

        esp_hid_scan_results_free(results);

        if (connecting) {
            // Let the OPEN/CLOSE callbacks handle the next scan
            break;
        }
        if (s_scanning) {
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
    s_scan_task = nullptr;
    vTaskDelete(nullptr);
}

static void start_scan_task(void)
{
    xTaskCreate(scan_task, "hid_scan", 6 * 1024, nullptr, 2, &s_scan_task);
}

// ---- Public API ----
void hid_gamepad_init(void *state_ptr)
{
    s_state = (VehicleState *)state_ptr;

    ESP_LOGI(TAG, "Initializing Bluetooth HID host (mode=%d)", HID_HOST_MODE);
    ESP_ERROR_CHECK(esp_hid_gap_init(HID_HOST_MODE));

#if CONFIG_BT_BLE_ENABLED
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler));
#endif

    esp_hidh_config_t config = {
        .callback = hidh_callback,
        .event_stack_size = 4096,
        .callback_arg = nullptr,
    };
    ESP_ERROR_CHECK(esp_hidh_init(&config));

    ESP_LOGI(TAG, "HID host ready");
}

void hid_gamepad_set_scanning(bool enabled)
{
    s_scanning = enabled;
    if (enabled && s_scan_task == nullptr) {
        start_scan_task();
    }
}
