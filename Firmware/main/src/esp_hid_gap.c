/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 *
 * Adapted from IDF examples/bluetooth/esp_hid_host for this project.
 * SSP is disabled for simpler gamepad pairing (legacy pairing / just-works).
 */

#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_hid_gap.h"
#if CONFIG_BT_BLUEDROID_ENABLED
#include "esp_bt_device.h"
#endif

static const char *TAG = "ESP_HID_GAP";

#define GAP_DBG_PRINTF(...) ESP_LOGD(TAG, __VA_ARGS__)

#if CONFIG_BT_HID_HOST_ENABLED
static const char *gap_bt_prop_type_names[5] = {"", "BDNAME", "COD", "RSSI", "EIR"};
#endif

static esp_hid_scan_result_t *bt_scan_results = NULL;
static size_t num_bt_scan_results = 0;

static esp_hid_scan_result_t *ble_scan_results = NULL;
static size_t num_ble_scan_results = 0;

static SemaphoreHandle_t bt_hidh_cb_semaphore = NULL;
#define WAIT_BT_CB()  xSemaphoreTake(bt_hidh_cb_semaphore, portMAX_DELAY)
#define SEND_BT_CB()  xSemaphoreGive(bt_hidh_cb_semaphore)

static SemaphoreHandle_t ble_hidh_cb_semaphore = NULL;
#define WAIT_BLE_CB() xSemaphoreTake(ble_hidh_cb_semaphore, portMAX_DELAY)
#define SEND_BLE_CB() xSemaphoreGive(ble_hidh_cb_semaphore)

#define SIZEOF_ARRAY(a) (sizeof(a) / sizeof(*a))

#if !CONFIG_BT_NIMBLE_ENABLED
static const char *ble_gap_evt_names[] = {
    "ADV_DATA_SET_COMPLETE", "SCAN_RSP_DATA_SET_COMPLETE", "SCAN_PARAM_SET_COMPLETE",
    "SCAN_RESULT", "ADV_DATA_RAW_SET_COMPLETE", "SCAN_RSP_DATA_RAW_SET_COMPLETE",
    "ADV_START_COMPLETE", "SCAN_START_COMPLETE", "AUTH_CMPL", "KEY", "SEC_REQ",
    "PASSKEY_NOTIF", "PASSKEY_REQ", "OOB_REQ", "LOCAL_IR", "LOCAL_ER", "NC_REQ",
    "ADV_STOP_COMPLETE", "SCAN_STOP_COMPLETE", "SET_STATIC_RAND_ADDR",
    "UPDATE_CONN_PARAMS", "SET_PKT_LENGTH_COMPLETE", "SET_LOCAL_PRIVACY_COMPLETE",
    "REMOVE_BOND_DEV_COMPLETE", "CLEAR_BOND_DEV_COMPLETE", "GET_BOND_DEV_COMPLETE",
    "READ_RSSI_COMPLETE", "UPDATE_WHITELIST_COMPLETE"
};
static const char *bt_gap_evt_names[] = {
    "DISC_RES", "DISC_STATE_CHANGED", "RMT_SRVCS", "RMT_SRVC_REC", "AUTH_CMPL",
    "PIN_REQ", "CFM_REQ", "KEY_NOTIF", "KEY_REQ", "READ_RSSI_DELTA"
};
static const char *ble_addr_type_names[] = {"PUBLIC", "RANDOM", "RPA_PUBLIC", "RPA_RANDOM"};

const char *ble_addr_type_str(esp_ble_addr_type_t ble_addr_type)
{
    if (ble_addr_type > BLE_ADDR_TYPE_RPA_RANDOM) {
        return "UNKNOWN";
    }
    return ble_addr_type_names[ble_addr_type];
}

static const char *ble_gap_evt_str(uint8_t event)
{
    if (event >= SIZEOF_ARRAY(ble_gap_evt_names)) {
        return "UNKNOWN";
    }
    return ble_gap_evt_names[event];
}

static const char *bt_gap_evt_str(uint8_t event)
{
    if (event >= SIZEOF_ARRAY(bt_gap_evt_names)) {
        return "UNKNOWN";
    }
    return bt_gap_evt_names[event];
}
#endif  /* !CONFIG_BT_NIMBLE_ENABLED */

#if CONFIG_BT_BLE_ENABLED
static const char *esp_ble_key_type_str(esp_ble_key_type_t key_type)
{
    switch (key_type) {
    case ESP_LE_KEY_NONE:   return "NONE";
    case ESP_LE_KEY_PENC:   return "PENC";
    case ESP_LE_KEY_PID:    return "PID";
    case ESP_LE_KEY_PCSRK:  return "PCSRK";
    case ESP_LE_KEY_PLK:    return "PLK";
    case ESP_LE_KEY_LLK:    return "LLK";
    case ESP_LE_KEY_LENC:   return "LENC";
    case ESP_LE_KEY_LID:    return "LID";
    case ESP_LE_KEY_LCSRK:  return "LCSRK";
    default:                return "UNKNOWN";
    }
}
#endif /* CONFIG_BT_BLE_ENABLED */

void esp_hid_scan_results_free(esp_hid_scan_result_t *results)
{
    esp_hid_scan_result_t *r = NULL;
    while (results) {
        r = results;
        results = results->next;
        if (r->name != NULL) {
            free((char *)r->name);
        }
        free(r);
    }
}

#if (CONFIG_BT_HID_HOST_ENABLED || CONFIG_BT_BLE_ENABLED)
static esp_hid_scan_result_t *find_scan_result(esp_bd_addr_t bda, esp_hid_scan_result_t *results)
{
    esp_hid_scan_result_t *r = results;
    while (r) {
        if (memcmp(bda, r->bda, sizeof(esp_bd_addr_t)) == 0) {
            return r;
        }
        r = r->next;
    }
    return NULL;
}
#endif

#if CONFIG_BT_HID_HOST_ENABLED
static void add_bt_scan_result(esp_bd_addr_t bda, esp_bt_cod_t *cod, esp_bt_uuid_t *uuid,
                                uint8_t *name, uint8_t name_len, int rssi)
{
    esp_hid_scan_result_t *r = find_scan_result(bda, bt_scan_results);
    if (r) {
        if (r->name == NULL && name && name_len) {
            char *name_s = (char *)malloc(name_len + 1);
            if (name_s == NULL) { return; }
            memcpy(name_s, name, name_len);
            name_s[name_len] = 0;
            r->name = (const char *)name_s;
        }
        if (r->bt.uuid.len == 0 && uuid->len) {
            memcpy(&r->bt.uuid, uuid, sizeof(esp_bt_uuid_t));
        }
        if (rssi != 0) { r->rssi = rssi; }
        return;
    }
    r = (esp_hid_scan_result_t *)malloc(sizeof(esp_hid_scan_result_t));
    if (r == NULL) { return; }
    r->transport = ESP_HID_TRANSPORT_BT;
    memcpy(r->bda, bda, sizeof(esp_bd_addr_t));
    memcpy(&r->bt.cod, cod, sizeof(esp_bt_cod_t));
    memcpy(&r->bt.uuid, uuid, sizeof(esp_bt_uuid_t));
    r->usage = esp_hid_usage_from_cod((uint32_t)cod);
    r->rssi = rssi;
    r->name = NULL;
    if (name_len && name) {
        char *name_s = (char *)malloc(name_len + 1);
        if (name_s == NULL) { free(r); return; }
        memcpy(name_s, name, name_len);
        name_s[name_len] = 0;
        r->name = (const char *)name_s;
    }
    r->next = bt_scan_results;
    bt_scan_results = r;
    num_bt_scan_results++;
}
#endif /* CONFIG_BT_HID_HOST_ENABLED */

#if CONFIG_BT_BLE_ENABLED
static void add_ble_scan_result(esp_bd_addr_t bda, esp_ble_addr_type_t addr_type,
                                 uint16_t appearance, uint8_t *name, uint8_t name_len, int rssi)
{
    if (find_scan_result(bda, ble_scan_results)) {
        return;
    }
    esp_hid_scan_result_t *r = (esp_hid_scan_result_t *)malloc(sizeof(esp_hid_scan_result_t));
    if (r == NULL) { return; }
    r->transport = ESP_HID_TRANSPORT_BLE;
    memcpy(r->bda, bda, sizeof(esp_bd_addr_t));
    r->ble.appearance = appearance;
    r->ble.addr_type = addr_type;
    r->usage = esp_hid_usage_from_appearance(appearance);
    r->rssi = rssi;
    r->name = NULL;
    if (name_len && name) {
        char *name_s = (char *)malloc(name_len + 1);
        if (name_s == NULL) { free(r); return; }
        memcpy(name_s, name, name_len);
        name_s[name_len] = 0;
        r->name = (const char *)name_s;
    }
    r->next = ble_scan_results;
    ble_scan_results = r;
    num_ble_scan_results++;
}
#endif /* CONFIG_BT_BLE_ENABLED */

#if !CONFIG_BT_NIMBLE_ENABLED
void print_uuid(esp_bt_uuid_t *uuid)
{
    if (uuid->len == ESP_UUID_LEN_16) {
        GAP_DBG_PRINTF("UUID16: 0x%04x", uuid->uuid.uuid16);
    } else if (uuid->len == ESP_UUID_LEN_32) {
        GAP_DBG_PRINTF("UUID32: 0x%08" PRIx32, uuid->uuid.uuid32);
    } else if (uuid->len == ESP_UUID_LEN_128) {
        GAP_DBG_PRINTF("UUID128: %02x%02x%02x%02x...",
                       uuid->uuid.uuid128[0], uuid->uuid.uuid128[1],
                       uuid->uuid.uuid128[2], uuid->uuid.uuid128[3]);
    }
}

#if CONFIG_BT_HID_HOST_ENABLED
static void handle_bt_device_result(struct disc_res_param *disc_res)
{
    uint32_t codv = 0;
    esp_bt_cod_t *cod = (esp_bt_cod_t *)&codv;
    int8_t rssi = 0;
    uint8_t *name = NULL;
    uint8_t name_len = 0;
    esp_bt_uuid_t uuid;

    uuid.len = ESP_UUID_LEN_16;
    uuid.uuid.uuid16 = 0;

    for (int i = 0; i < disc_res->num_prop; i++) {
        esp_bt_gap_dev_prop_t *prop = &disc_res->prop[i];
        if (prop->type == ESP_BT_GAP_DEV_PROP_BDNAME) {
            name = (uint8_t *)prop->val;
            name_len = strlen((const char *)name);
        } else if (prop->type == ESP_BT_GAP_DEV_PROP_RSSI) {
            rssi = *((int8_t *)prop->val);
        } else if (prop->type == ESP_BT_GAP_DEV_PROP_COD) {
            memcpy(&codv, prop->val, sizeof(uint32_t));
        } else if (prop->type == ESP_BT_GAP_DEV_PROP_EIR) {
            uint8_t len = 0;
            uint8_t *data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val,
                                                         ESP_BT_EIR_TYPE_CMPL_16BITS_UUID, &len);
            if (data == NULL) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val,
                                                    ESP_BT_EIR_TYPE_INCMPL_16BITS_UUID, &len);
            }
            if (data && len == ESP_UUID_LEN_16) {
                uuid.len = ESP_UUID_LEN_16;
                uuid.uuid.uuid16 = data[0] + (data[1] << 8);
            }
            if (name == NULL) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val,
                                                    ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);
                if (data == NULL) {
                    data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val,
                                                        ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &len);
                }
                if (data && len) {
                    name = data;
                    name_len = len;
                }
            }
        }
    }

    if (cod->major == ESP_BT_COD_MAJOR_DEV_PERIPHERAL ||
        (find_scan_result(disc_res->bda, bt_scan_results) != NULL)) {
        add_bt_scan_result(disc_res->bda, cod, &uuid, name, name_len, rssi);
    }
}
#endif  /* CONFIG_BT_HID_HOST_ENABLED */

#if CONFIG_BT_BLE_ENABLED
static void handle_ble_device_result(struct ble_scan_result_evt_param *scan_rst)
{
    uint16_t uuid = 0;
    uint16_t appearance = 0;
    char name[64] = {0};

    uint8_t uuid_len = 0;
    uint8_t *uuid_d = esp_ble_resolve_adv_data_by_type(scan_rst->ble_adv,
                                               scan_rst->adv_data_len + scan_rst->scan_rsp_len,
                                               ESP_BLE_AD_TYPE_16SRV_CMPL, &uuid_len);
    if (uuid_d && uuid_len) {
        uuid = uuid_d[0] + (uuid_d[1] << 8);
    }

    uint8_t appearance_len = 0;
    uint8_t *appearance_d = esp_ble_resolve_adv_data_by_type(scan_rst->ble_adv,
                                                    scan_rst->adv_data_len + scan_rst->scan_rsp_len,
                                                    ESP_BLE_AD_TYPE_APPEARANCE, &appearance_len);
    if (appearance_d && appearance_len) {
        appearance = appearance_d[0] + (appearance_d[1] << 8);
    }

    uint8_t adv_name_len = 0;
    uint8_t *adv_name = esp_ble_resolve_adv_data_by_type(scan_rst->ble_adv,
                                                 scan_rst->adv_data_len + scan_rst->scan_rsp_len,
                                                 ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
    if (adv_name == NULL) {
        adv_name = esp_ble_resolve_adv_data_by_type(scan_rst->ble_adv,
                                            scan_rst->adv_data_len + scan_rst->scan_rsp_len,
                                            ESP_BLE_AD_TYPE_NAME_SHORT, &adv_name_len);
    }
    if (adv_name && adv_name_len) {
        memcpy(name, adv_name, adv_name_len < 63 ? adv_name_len : 63);
    }

    if (uuid == ESP_GATT_UUID_HID_SVC) {
        add_ble_scan_result(scan_rst->bda, scan_rst->ble_addr_type, appearance,
                            adv_name, adv_name_len, scan_rst->rssi);
    }
}
#endif /* CONFIG_BT_BLE_ENABLED */

#if CONFIG_BT_HID_HOST_ENABLED
static void bt_gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            SEND_BT_CB();
        }
        break;
    case ESP_BT_GAP_DISC_RES_EVT:
        handle_bt_device_result(&param->disc_res);
        break;
    case ESP_BT_GAP_CFM_REQ_EVT:
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(TAG, "BT GAP mode change: %d", param->mode_chg.mode);
        break;
    case ESP_BT_GAP_PIN_REQ_EVT:
        if (param->pin_req.min_16_digit) {
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            esp_bt_pin_code_t pin_code = {'1', '2', '3', '4'};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    default:
        ESP_LOGV(TAG, "BT GAP event %s", bt_gap_evt_str(event));
        break;
    }
}

static esp_err_t init_bt_gap(void)
{
    esp_err_t ret;
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    if ((ret = esp_bt_gap_register_callback(bt_gap_event_handler)) != ESP_OK) {
        ESP_LOGE(TAG, "bt_gap_register_callback failed: %d", ret);
        return ret;
    }
    if ((ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE)) != ESP_OK) {
        ESP_LOGE(TAG, "bt_gap_set_scan_mode failed: %d", ret);
        return ret;
    }
    return ESP_OK;
}

static esp_err_t start_bt_scan(uint32_t seconds)
{
    return esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, (int)(seconds / 1.28), 0);
}
#endif /* CONFIG_BT_HID_HOST_ENABLED */

#if CONFIG_BT_BLE_ENABLED
static void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        SEND_BLE_CB();
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        switch (param->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            handle_ble_device_result(&param->scan_rst);
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            SEND_BLE_CB();
            break;
        default:
            break;
        }
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (!param->ble_security.auth_cmpl.success) {
            ESP_LOGE(TAG, "BLE auth error: 0x%x", param->ble_security.auth_cmpl.fail_reason);
        } else {
            ESP_LOGI(TAG, "BLE auth success");
        }
        break;
    case ESP_GAP_BLE_KEY_EVT:
        ESP_LOGI(TAG, "BLE key type = %s", esp_ble_key_type_str(param->ble_security.ble_key.key_type));
        break;
    case ESP_GAP_BLE_NC_REQ_EVT:
        esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    default:
        ESP_LOGV(TAG, "BLE GAP event %s", ble_gap_evt_str(event));
        break;
    }
}

static esp_err_t init_ble_gap(void)
{
    return esp_ble_gap_register_callback(ble_gap_event_handler);
}

static esp_ble_scan_params_t hid_scan_params = {
    .scan_type          = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval      = 0x50,
    .scan_window        = 0x30,
    .scan_duplicate     = BLE_SCAN_DUPLICATE_ENABLE,
};

static esp_err_t start_ble_scan(uint32_t seconds)
{
    esp_err_t ret = esp_ble_gap_set_scan_params(&hid_scan_params);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ble_gap_set_scan_params failed: %d", ret);
        return ret;
    }
    WAIT_BLE_CB();
    return esp_ble_gap_start_scanning(seconds);
}
#endif /* CONFIG_BT_BLE_ENABLED */

static esp_err_t init_low_level(uint8_t mode)
{
    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
#if CONFIG_IDF_TARGET_ESP32
    bt_cfg.mode = mode;
#endif
#if CONFIG_BT_HID_HOST_ENABLED
    if (mode & ESP_BT_MODE_CLASSIC_BT) {
        bt_cfg.bt_max_acl_conn = 3;
        bt_cfg.bt_max_sync_conn = 3;
    } else
#endif
    {
        ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
        if (ret) {
            ESP_LOGE(TAG, "mem_release failed: %d", ret);
            return ret;
        }
    }
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "bt_controller_init failed: %d", ret);
        return ret;
    }
    ret = esp_bt_controller_enable(mode);
    if (ret) {
        ESP_LOGE(TAG, "bt_controller_enable failed: %d", ret);
        return ret;
    }

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    bluedroid_cfg.ssp_en = false;  // Legacy pairing for simpler gamepad connection
    ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if (ret) {
        ESP_LOGE(TAG, "bluedroid_init failed: %d", ret);
        return ret;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "bluedroid_enable failed: %d", ret);
        return ret;
    }
#if CONFIG_BT_HID_HOST_ENABLED
    if (mode & ESP_BT_MODE_CLASSIC_BT) {
        ret = init_bt_gap();
        if (ret) { return ret; }
    }
#endif
#if CONFIG_BT_BLE_ENABLED
    if (mode & ESP_BT_MODE_BLE) {
        ret = init_ble_gap();
        if (ret) { return ret; }
    }
#endif
    return ESP_OK;
}
#endif  /* !CONFIG_BT_NIMBLE_ENABLED */

esp_err_t esp_hid_gap_init(uint8_t mode)
{
    if (!mode || mode > ESP_BT_MODE_BTDM) {
        ESP_LOGE(TAG, "Invalid mode: %d", mode);
        return ESP_FAIL;
    }
    if (bt_hidh_cb_semaphore != NULL) {
        ESP_LOGE(TAG, "Already initialised");
        return ESP_FAIL;
    }
    bt_hidh_cb_semaphore = xSemaphoreCreateBinary();
    if (bt_hidh_cb_semaphore == NULL) {
        ESP_LOGE(TAG, "Failed to create BT semaphore");
        return ESP_FAIL;
    }
    ble_hidh_cb_semaphore = xSemaphoreCreateBinary();
    if (ble_hidh_cb_semaphore == NULL) {
        vSemaphoreDelete(bt_hidh_cb_semaphore);
        bt_hidh_cb_semaphore = NULL;
        ESP_LOGE(TAG, "Failed to create BLE semaphore");
        return ESP_FAIL;
    }
    esp_err_t ret = init_low_level(mode);
    if (ret != ESP_OK) {
        vSemaphoreDelete(bt_hidh_cb_semaphore);
        bt_hidh_cb_semaphore = NULL;
        vSemaphoreDelete(ble_hidh_cb_semaphore);
        ble_hidh_cb_semaphore = NULL;
    }
    return ret;
}

esp_err_t esp_hid_scan(uint32_t seconds, size_t *num_results, esp_hid_scan_result_t **results)
{
    if (num_bt_scan_results || bt_scan_results || num_ble_scan_results || ble_scan_results) {
        ESP_LOGE(TAG, "Old scan results pending — free them first");
        return ESP_FAIL;
    }

#if CONFIG_BT_BLE_ENABLED
    if (start_ble_scan(seconds) == ESP_OK) {
        WAIT_BLE_CB();
    } else {
        return ESP_FAIL;
    }
#endif
#if CONFIG_BT_HID_HOST_ENABLED
    if (start_bt_scan(seconds) == ESP_OK) {
        WAIT_BT_CB();
    } else {
        return ESP_FAIL;
    }
#endif

    *num_results = num_bt_scan_results + num_ble_scan_results;
    *results = bt_scan_results;
    if (num_bt_scan_results && bt_scan_results) {
        esp_hid_scan_result_t *last = bt_scan_results;
        while (last->next) { last = last->next; }
        last->next = ble_scan_results;
    } else {
        *results = ble_scan_results;
    }

    num_bt_scan_results = 0;
    bt_scan_results = NULL;
    num_ble_scan_results = 0;
    ble_scan_results = NULL;
    return ESP_OK;
}
