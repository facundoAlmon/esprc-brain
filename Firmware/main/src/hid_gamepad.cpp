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
// Layout confirmado desde reports capturados:
//   Bytes  0-1:  Left  stick X  (uint16 LE, 0=izq, 32767=centro, 65535=der)
//   Bytes  2-3:  Left  stick Y  (uint16 LE, 0=arriba, 32767=centro, 65535=abajo)
//   Bytes  4-5:  Right stick X
//   Bytes  6-7:  Right stick Y
//   Bytes  8-9:  Left  trigger  (uint16 0-1023)
//   Bytes 10-11: Right trigger  (uint16 0-1023)
//   Bytes 12-13: Botones + HAT  (uint16 LE):
//     bits 3:0 = HAT  (0=nada,1=N,2=NE,3=E,4=SE,5=S,6=SW,7=W,8=NW)
//     bit   8  = A
//     bit   9  = B
//     bit  11  = X
//     bit  12  = Y
//     bit  14  = LB
//     bit  15  = RB
//   Byte 14: bit2=View  bit3=Menu  bit5=LS  bit6=RS
//   Byte 15: bit0=Capture
static void parse_xbox(GamepadData *gp, const uint8_t *d, uint16_t len)
{
    if (len < 14) return;

    // Sticks: uint16 centrado en 32767 → rango -512..+512
    uint16_t lx = (uint16_t)d[0] | ((uint16_t)d[1] << 8);
    uint16_t ly = (uint16_t)d[2] | ((uint16_t)d[3] << 8);
    uint16_t rx = (uint16_t)d[4] | ((uint16_t)d[5] << 8);
    uint16_t ry = (uint16_t)d[6] | ((uint16_t)d[7] << 8);
    gp->axis_x  = (int16_t)((int32_t)((int32_t)lx - 32767) * 512 / 32767);
    gp->axis_y  = (int16_t)((int32_t)((int32_t)ly - 32767) * 512 / 32767);
    gp->axis_rx = (int16_t)((int32_t)((int32_t)rx - 32767) * 512 / 32767);
    gp->axis_ry = (int16_t)((int32_t)((int32_t)ry - 32767) * 512 / 32767);

    // Triggers
    gp->brake    = (uint16_t)d[8]  | ((uint16_t)d[9]  << 8);   // LT
    gp->throttle = (uint16_t)d[10] | ((uint16_t)d[11] << 8);   // RT

    // Botones en bytes 12-13 (confirmados con datos capturados)
    uint16_t btns = (uint16_t)d[12] | ((uint16_t)d[13] << 8);
    gp->dpad    = btns & 0x0F;  // HAT: 0=none,1=N,2=NE,3=E,4=SE,5=S,6=SW,7=W,8=NW
    gp->buttons = 0;
    if (btns & (1u <<  8)) gp->buttons |= GAMEPAD_BTN_A;
    if (btns & (1u <<  9)) gp->buttons |= GAMEPAD_BTN_B;
    if (btns & (1u << 11)) gp->buttons |= GAMEPAD_BTN_X;
    if (btns & (1u << 12)) gp->buttons |= GAMEPAD_BTN_Y;
    if (btns & (1u << 14)) gp->buttons |= GAMEPAD_BTN_L1;   // LB
    if (btns & (1u << 15)) gp->buttons |= GAMEPAD_BTN_R1;   // RB

    // Botones en byte 14 (confirmados con datos capturados)
    if (len > 14) {
        uint8_t b14 = d[14];
        if (b14 & (1u << 2)) gp->buttons |= GAMEPAD_BTN_SELECT;   // View (⧉)
        if (b14 & (1u << 5)) gp->buttons |= GAMEPAD_BTN_THUMB_L;  // LS
        if (b14 & (1u << 6)) gp->buttons |= GAMEPAD_BTN_THUMB_R;  // RS
    }
    // Byte 15 bit 0 = Capture/Share — no usado en gamepadHandler
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

    // Raw bytes logged at VERBOSE so you can capture them with
    //   idf.py monitor | grep "GP raw"
    // and verify the layout matches your gamepad.
    ESP_LOGV(TAG, "GP raw[%d]: %02x %02x %02x %02x %02x %02x %02x %02x",
             len, d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);

    gp->axis_x  = (int16_t)((int32_t)((int16_t)d[0] - 128) * 512 / 127);
    gp->axis_y  = (int16_t)((int32_t)((int16_t)d[1] - 128) * 512 / 127);
    gp->axis_rx = (int16_t)((int32_t)((int16_t)d[2] - 128) * 512 / 127);
    gp->axis_ry = (int16_t)((int32_t)((int16_t)d[3] - 128) * 512 / 127);
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
    if (btn & (1u << 8)) gp->buttons |= GAMEPAD_BTN_SELECT;   // Select/Back
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
