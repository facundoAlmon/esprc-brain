#include "webServerHandler.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "dns_server.h"
#include "hid_gamepad.h"
#include "actuators.h"
#include "state.h"
#include "ProgramManager.h"
#include <ArduinoJson.h>
#include <string>
#include <string.h>

static const char* TAG = "WebServer";

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static inline uint32_t millis() { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

static const char* NVS_NS = "bl-car";

// ---- NVS helpers (local) ----

static uint32_t nvs_get_u32_or(const char* key, uint32_t def) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return def;
    uint32_t v = def; nvs_get_u32(h, key, &v);
    nvs_close(h); return v;
}

static bool nvs_get_bool_or(const char* key, bool def) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return def;
    uint8_t v = def ? 1 : 0; nvs_get_u8(h, key, &v);
    nvs_close(h); return v != 0;
}

static std::string nvs_get_str_or(const char* key, const char* def) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return def ? def : "";
    size_t sz = 0;
    if (nvs_get_str(h, key, nullptr, &sz) != ESP_OK || sz == 0) { nvs_close(h); return def ? def : ""; }
    std::string result(sz, '\0');
    nvs_get_str(h, key, &result[0], &sz);
    nvs_close(h);
    if (!result.empty() && result.back() == '\0') result.pop_back();
    return result;
}

static bool nvs_put_u32(const char* key, uint32_t v) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_set_u32(h, key, v) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h); return ok;
}

static bool nvs_put_bool(const char* key, bool v) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_set_u8(h, key, v ? 1 : 0) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h); return ok;
}

static bool nvs_put_str(const char* key, const char* value) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_set_str(h, key, value ? value : "") == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h); return ok;
}

static bool nvs_str_equals(const char* key, const char* compare) {
    std::string s = nvs_get_str_or(key, "");
    return s == compare;
}

static bool nvs_clear_all() {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_erase_all(h) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h); return ok;
}

// ---- Server state ----

static httpd_handle_t server_httpd = NULL;
static VehicleState* g_state = nullptr;
static ProgramManager* g_programManager = nullptr;

// Forward declarations
static esp_err_t get_index_handler(httpd_req_t *req);
static esp_err_t get_config_handler(httpd_req_t *req);
static esp_err_t post_config_handler(httpd_req_t *req);
static esp_err_t get_led_config_handler(httpd_req_t *req);
static esp_err_t post_led_config_handler(httpd_req_t *req);
static esp_err_t get_wifi_handler(httpd_req_t *req);
static esp_err_t post_wifi_handler(httpd_req_t *req);
static esp_err_t post_manage_handler(httpd_req_t *req);
static esp_err_t post_act_handler(httpd_req_t *req);
static esp_err_t handle_ws_req(httpd_req_t *req);
static esp_err_t cors_handler(httpd_req_t* req);
static void act_from_json(JsonDocument& doc, VehicleState* state);
static esp_err_t post_program_handler(httpd_req_t *req);
static esp_err_t get_program_run_handler(httpd_req_t *req);
static esp_err_t get_program_stop_handler(httpd_req_t *req);
static esp_err_t get_program_clear_handler(httpd_req_t *req);
static esp_err_t get_program_handler(httpd_req_t *req);
static esp_err_t get_recording_start_handler(httpd_req_t *req);
static esp_err_t get_recording_stop_handler(httpd_req_t *req);
static esp_err_t get_config_backup_handler(httpd_req_t *req);
static esp_err_t post_config_restore_handler(httpd_req_t *req);
static esp_err_t post_sequence_handler(httpd_req_t *req);
static esp_err_t get_sequence_stop_handler(httpd_req_t *req);
static esp_err_t get_ota_info_handler(httpd_req_t *req);
static esp_err_t post_ota_handler(httpd_req_t *req);

static TaskHandle_t sequenceTaskHandle = NULL;
static StaticJsonDocument<1024> sequenceCommands;
static int sequenceIterations = 0;
static bool isSequenceRunning = false;

void sequenceTask(void *pvParameters);

// ---- JSON builders ----

static char* get_config_json() {
    JsonDocument doc;
    doc["servoCenterDeg"] = g_state->servoCenterDeg;
    doc["servoLimitLDeg"] = g_state->servoLimitLDeg;
    doc["servoLimitRDeg"] = g_state->servoLimitRDeg;
    doc["motorMaxSpeed"] = g_state->motorMaxSpeed;
    doc["motorMinSpeed"] = g_state->motorMinSpeed;
    doc["enableScan"] = g_state->enableScan;
    doc["autoTurnSignals"] = g_state->autoTurnSignals;
    doc["autoTurnTol"] = g_state->autoTurnTol;
    char *json_string = (char*)malloc(512);
    serializeJson(doc, json_string, 512);
    return json_string;
}

// ---- startServer ----

void startServer(VehicleState* state, ProgramManager* programManager) {
    g_state = state;
    g_programManager = programManager;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 24;
    config.recv_wait_timeout = 30;
    config.send_wait_timeout = 30;
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.global_user_ctx = g_state;

    // mDNS
    mdns_init();
    mdns_hostname_set("ecar");
    mdns_service_add("ESP-RC-CAR", "_http", "_tcp", 80, NULL, 0);

    // Captive portal DNS in AP mode
    if (strcmp(g_state->wifiMode, "AP") == 0) {
        dns_server_start(g_state->espIP);
    }

    ESP_LOGI(TAG, "Iniciando servidor web en el puerto: %d", config.server_port);

    if (httpd_start(&server_httpd, &config) == ESP_OK) {
        httpd_uri_t get_index_uri = {.uri = "/", .method = HTTP_GET, .handler = get_index_handler};
        httpd_register_uri_handler(server_httpd, &get_index_uri);
        httpd_uri_t cors_uri = {.uri = "*", .method = HTTP_OPTIONS, .handler = cors_handler};
        httpd_register_uri_handler(server_httpd, &cors_uri);
        httpd_uri_t get_config_uri = {.uri = "/config", .method = HTTP_GET, .handler = get_config_handler};
        httpd_register_uri_handler(server_httpd, &get_config_uri);
        httpd_uri_t post_config_uri = {.uri = "/config", .method = HTTP_POST, .handler = post_config_handler};
        httpd_register_uri_handler(server_httpd, &post_config_uri);
        httpd_uri_t get_led_config_uri = {.uri = "/api/leds", .method = HTTP_GET, .handler = get_led_config_handler};
        httpd_register_uri_handler(server_httpd, &get_led_config_uri);
        httpd_uri_t post_led_config_uri = {.uri = "/api/leds", .method = HTTP_POST, .handler = post_led_config_handler};
        httpd_register_uri_handler(server_httpd, &post_led_config_uri);
        httpd_uri_t get_wifi_uri = {.uri = "/wifi", .method = HTTP_GET, .handler = get_wifi_handler};
        httpd_register_uri_handler(server_httpd, &get_wifi_uri);
        httpd_uri_t post_wifi_uri = {.uri = "/wifi", .method = HTTP_POST, .handler = post_wifi_handler};
        httpd_register_uri_handler(server_httpd, &post_wifi_uri);
        httpd_uri_t post_manage_uri = {.uri = "/manage", .method = HTTP_POST, .handler = post_manage_handler};
        httpd_register_uri_handler(server_httpd, &post_manage_uri);
        httpd_uri_t post_act_uri = {.uri = "/act", .method = HTTP_POST, .handler = post_act_handler};
        httpd_register_uri_handler(server_httpd, &post_act_uri);
        httpd_uri_t ws_uri = {.uri = "/ws", .method = HTTP_GET, .handler = handle_ws_req, .is_websocket = true};
        httpd_register_uri_handler(server_httpd, &ws_uri);
        httpd_uri_t post_program_uri = {.uri = "/api/program", .method = HTTP_POST, .handler = post_program_handler};
        httpd_register_uri_handler(server_httpd, &post_program_uri);
        httpd_uri_t get_program_run_uri = {.uri = "/api/program/run", .method = HTTP_GET, .handler = get_program_run_handler};
        httpd_register_uri_handler(server_httpd, &get_program_run_uri);
        httpd_uri_t get_program_stop_uri = {.uri = "/api/program/stop", .method = HTTP_GET, .handler = get_program_stop_handler};
        httpd_register_uri_handler(server_httpd, &get_program_stop_uri);
        httpd_uri_t get_program_clear_uri = {.uri = "/api/program/clear", .method = HTTP_GET, .handler = get_program_clear_handler};
        httpd_register_uri_handler(server_httpd, &get_program_clear_uri);
        httpd_uri_t get_program_uri = {.uri = "/api/program", .method = HTTP_GET, .handler = get_program_handler};
        httpd_register_uri_handler(server_httpd, &get_program_uri);
        httpd_uri_t get_recording_start_uri = {.uri = "/api/recording/start", .method = HTTP_GET, .handler = get_recording_start_handler};
        httpd_register_uri_handler(server_httpd, &get_recording_start_uri);
        httpd_uri_t get_recording_stop_uri = {.uri = "/api/recording/stop", .method = HTTP_GET, .handler = get_recording_stop_handler};
        httpd_register_uri_handler(server_httpd, &get_recording_stop_uri);
        httpd_uri_t get_config_backup_uri = {.uri = "/api/config/backup", .method = HTTP_GET, .handler = get_config_backup_handler};
        httpd_register_uri_handler(server_httpd, &get_config_backup_uri);
        httpd_uri_t post_config_restore_uri = {.uri = "/api/config/restore", .method = HTTP_POST, .handler = post_config_restore_handler};
        httpd_register_uri_handler(server_httpd, &post_config_restore_uri);
        httpd_uri_t post_sequence_uri = {.uri = "/api/sequence", .method = HTTP_POST, .handler = post_sequence_handler};
        httpd_register_uri_handler(server_httpd, &post_sequence_uri);
        httpd_uri_t get_sequence_stop_uri = {.uri = "/api/sequence/stop", .method = HTTP_GET, .handler = get_sequence_stop_handler};
        httpd_register_uri_handler(server_httpd, &get_sequence_stop_uri);
        httpd_uri_t get_ota_info_uri = {.uri = "/api/ota/info", .method = HTTP_GET, .handler = get_ota_info_handler};
        httpd_register_uri_handler(server_httpd, &get_ota_info_uri);
        httpd_uri_t post_ota_uri = {.uri = "/api/ota", .method = HTTP_POST, .handler = post_ota_handler};
        httpd_register_uri_handler(server_httpd, &post_ota_uri);
    }
}

void stopServer() {
    if (server_httpd) { httpd_stop(server_httpd); server_httpd = NULL; }
}

// ---- Handlers ----

static esp_err_t get_index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    extern const uint8_t html_file_array[] asm("_binary_index_html_start");
    extern const uint8_t html_file_array_end[] asm("_binary_index_html_end");
    size_t html_file_size = (html_file_array_end - html_file_array);
    return httpd_resp_send(req, (const char *)html_file_array, html_file_size);
}

static esp_err_t get_config_handler(httpd_req_t *req) {
    char *my_json_string = get_config_json();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, my_json_string, strlen(my_json_string));
    free(my_json_string);
    return ESP_OK;
}

#define MAX_POST_SIZE 4096

static esp_err_t post_config_handler(httpd_req_t *req) {
    char content[MAX_POST_SIZE];
    size_t recv_size = MIN(req->content_len, MAX_POST_SIZE);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) return ESP_FAIL;
    content[ret] = '\0';

    JsonDocument doc;
    deserializeJson(doc, content);

    VehicleState* state = (VehicleState*)httpd_get_global_user_ctx(req->handle);
    state->servoCenterDeg = doc["servoCenterDeg"];
    state->servoLimitLDeg = doc["servoLimitLDeg"];
    state->servoLimitRDeg = doc["servoLimitRDeg"];
    state->motorMaxSpeed  = doc["motorMaxSpeed"];
    state->motorMinSpeed  = doc["motorMinSpeed"];
    state->enableScan     = doc["enableScan"];
    if (doc["autoTurnSignals"] == 1) state->autoTurnSignals = true;
    else if (doc["autoTurnSignals"] == 0) state->autoTurnSignals = false;
    state->autoTurnTol = doc["autoTurnTol"];

    hid_gamepad_set_scanning(state->enableScan == 1);

    nvs_put_u32("servoCenterDeg", state->servoCenterDeg);
    nvs_put_u32("servoLimitLDeg", state->servoLimitLDeg);
    nvs_put_u32("servoLimitRDeg", state->servoLimitRDeg);
    nvs_put_u32("motorMaxSpeed",  state->motorMaxSpeed);
    nvs_put_u32("motorMinSpeed",  state->motorMinSpeed);
    nvs_put_u32("enableScan",     state->enableScan);
    nvs_put_bool("autoTurnSignals", state->autoTurnSignals);
    nvs_put_u32("autoTurnTol",    state->autoTurnTol);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t get_led_config_handler(httpd_req_t *req) {
    std::string ledConfigJson = nvs_get_str_or("ledConfig", "{\"total_leds\": 12, \"grupos\": []}");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, ledConfigJson.c_str(), ledConfigJson.length());
    return ESP_OK;
}

static esp_err_t post_led_config_handler(httpd_req_t *req) {
    char content[MAX_POST_SIZE];
    size_t recv_size = MIN(req->content_len, MAX_POST_SIZE);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) return ESP_FAIL;
    content[ret] = '\0';

    nvs_put_str("ledConfig", content);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "JSON malformado");
        return ESP_FAIL;
    }

    VehicleState* state = (VehicleState*)httpd_get_global_user_ctx(req->handle);
    state->ledCount = doc["total_leds"] | 12;
    JsonArray groups = doc["grupos"].as<JsonArray>();
    state->ledGroups.clear();
    for (JsonObject group : groups) {
        LedGroup newGroup;
        newGroup.funcion = (LedFunction)group["funcion"].as<int>();
        strncpy(newGroup.leds, group["leds"].as<const char*>(), sizeof(newGroup.leds));
        newGroup.colorR = group["color"]["r"];
        newGroup.colorG = group["color"]["g"];
        newGroup.colorB = group["color"]["b"];
        newGroup.brillo = group["brillo"];
        state->ledGroups.push_back(newGroup);
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t get_wifi_handler(httpd_req_t *req) {
    VehicleState* state = (VehicleState*)httpd_get_global_user_ctx(req->handle);
    JsonDocument doc;
    doc["wifiName"] = state->wifiName;
    doc["wifiPass"] = state->wifiPass;
    doc["wifiMode"] = state->wifiMode;
    doc["ip"] = state->espIP;
    doc["camIP"] = state->cameraIP;
    char json_string[256];
    serializeJson(doc, json_string, sizeof(json_string));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_string, strlen(json_string));
    return ESP_OK;
}

static esp_err_t post_wifi_handler(httpd_req_t *req) {
    char content[512];
    size_t recv_size = MIN(req->content_len, 512);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) return ESP_FAIL;
    content[ret] = '\0';

    JsonDocument doc;
    deserializeJson(doc, content);

    VehicleState* state = (VehicleState*)httpd_get_global_user_ctx(req->handle);
    strncpy(state->wifiName, doc["wifiName"] | "", sizeof(state->wifiName));
    strncpy(state->wifiPass, doc["wifiPass"] | "", sizeof(state->wifiPass));
    strncpy(state->wifiMode, doc["wifiMode"] | "", sizeof(state->wifiMode));

    nvs_put_str("wifiName", state->wifiName);
    nvs_put_str("wifiPass", state->wifiPass);
    nvs_put_str("wifiMode", state->wifiMode);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

static esp_err_t post_manage_handler(httpd_req_t *req) {
    char content[128];
    size_t recv_size = MIN(req->content_len, 128);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) return ESP_FAIL;
    content[ret] = '\0';

    JsonDocument doc;
    deserializeJson(doc, content);

    if (doc["clearPreferences"]) nvs_clear_all();

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);

    if (doc["restartESP"]) { vTaskDelay(1000 / portTICK_PERIOD_MS); esp_restart(); }
    return ESP_OK;
}

static void act_from_json(JsonDocument& doc, VehicleState* state) {
    bool has_actuator_cmd = doc.containsKey("motorSpeed") || doc.containsKey("motorDirection") ||
                            doc.containsKey("steerAng") || doc.containsKey("steerDirection");

    if (has_actuator_cmd) {
        state->apiActEnabled = true;
        state->apiActMSStart = millis();
        state->apiActMS = doc["ms"] | state->apiActMSTimeout;

        int motorSpeedRaw = doc["motorSpeed"] | 0;
        const char* motorDirectionStr = doc["motorDirection"] | "F";
        int motorSpeed = strcmp(motorDirectionStr, "F") == 0 ? motorSpeedRaw : -motorSpeedRaw;
        setMotor(motorSpeed, strcmp(motorDirectionStr, "F") == 0, state);

        int steerAngRaw = doc["steerAng"] | 0;
        const char* steerDirectionStr = doc["steerDirection"] | "R";
        int steerAngle = strcmp(steerDirectionStr, "L") == 0 ? -steerAngRaw : steerAngRaw;
        setSteer(steerAngle, state);

        if (g_programManager->isRecording()) g_programManager->recordStep(motorSpeed, steerAngle);
    }

    if (doc.containsKey("action")) {
        const char* action = doc["action"];
        if (strcmp(action, "headlights_cycle") == 0) {
            state->luces++; if (state->luces > 3) state->luces = 0;
        } else if (strcmp(action, "right_turn_toggle") == 0) {
            state->giroDerecho = !state->giroDerecho;
        } else if (strcmp(action, "left_turn_toggle") == 0) {
            state->giroIzquierdo = !state->giroIzquierdo;
        } else if (strcmp(action, "hazards_toggle") == 0) {
            state->baliza = !state->baliza;
        }
    }
}

static esp_err_t post_act_handler(httpd_req_t *req) {
    char content[256];
    size_t recv_size = MIN(req->content_len, 256);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) return ESP_FAIL;
    content[ret] = '\0';
    JsonDocument doc;
    deserializeJson(doc, content);
    act_from_json(doc, (VehicleState*)httpd_get_global_user_ctx(req->handle));
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t handle_ws_req(httpd_req_t *req) {
    if (req->method == HTTP_GET) return ESP_OK;
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) return ret;
    if (ws_pkt.len) {
        buf = (uint8_t*)calloc(1, ws_pkt.len + 1);
        if (!buf) return ESP_ERR_NO_MEM;
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) { free(buf); return ret; }
        if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
            JsonDocument doc;
            deserializeJson(doc, (char*)ws_pkt.payload);
            act_from_json(doc, (VehicleState*)httpd_get_global_user_ctx(req->handle));
        }
        free(buf);
    }
    return ESP_OK;
}

static esp_err_t cors_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t post_program_handler(httpd_req_t *req) {
    char content[MAX_POST_SIZE];
    size_t recv_size = MIN(req->content_len, MAX_POST_SIZE);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) return ESP_FAIL;
    content[ret] = '\0';
    JsonDocument doc;
    if (deserializeJson(doc, content)) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON"); return ESP_FAIL; }
    g_programManager->loadProgram(doc.as<JsonArray>());
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t get_program_run_handler(httpd_req_t *req) {
    int iterations = 1;
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char* buf = (char*)malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[32];
            if (httpd_query_key_value(buf, "iterations", param, sizeof(param)) == ESP_OK)
                iterations = atoi(param);
        }
        free(buf);
    }
    g_programManager->startProgram(iterations);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t get_program_stop_handler(httpd_req_t *req) {
    g_programManager->stopProgram();
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t get_program_clear_handler(httpd_req_t *req) {
    g_programManager->clearProgram();
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t get_program_handler(httpd_req_t *req) {
    JsonDocument doc = g_programManager->getProgramAsJson();
    std::string output;
    serializeJson(doc, output);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, output.c_str(), output.length());
    return ESP_OK;
}

static esp_err_t get_recording_start_handler(httpd_req_t *req) {
    g_programManager->startRecording();
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t get_recording_stop_handler(httpd_req_t *req) {
    g_programManager->stopRecording();
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t get_config_backup_handler(httpd_req_t *req) {
    JsonDocument doc;
    doc["servoCenterDeg"]  = nvs_get_u32_or("servoCenterDeg", 0);
    doc["servoLimitLDeg"]  = nvs_get_u32_or("servoLimitLDeg", 0);
    doc["servoLimitRDeg"]  = nvs_get_u32_or("servoLimitRDeg", 0);
    doc["motorMaxSpeed"]   = nvs_get_u32_or("motorMaxSpeed", 0);
    doc["motorMinSpeed"]   = nvs_get_u32_or("motorMinSpeed", 0);
    doc["enableScan"]      = nvs_get_u32_or("enableScan", 0);
    doc["autoTurnSignals"] = nvs_get_bool_or("autoTurnSignals", false);
    doc["autoTurnTol"]     = nvs_get_u32_or("autoTurnTol", 0);
    doc["ledConfig"]       = nvs_get_str_or("ledConfig", "{}");
    doc["wifiName"]        = nvs_get_str_or("wifiName", "");
    doc["wifiPass"]        = nvs_get_str_or("wifiPass", "");
    doc["wifiMode"]        = nvs_get_str_or("wifiMode", "");
    doc["camIP"]           = nvs_get_str_or("camIP", "");
    doc["program"]         = g_programManager->getProgramAsJson();

    std::string output;
    serializeJson(doc, output);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, output.c_str(), output.length());
    return ESP_OK;
}

static esp_err_t post_config_restore_handler(httpd_req_t *req) {
    char content[MAX_POST_SIZE];
    size_t recv_size = MIN(req->content_len, MAX_POST_SIZE);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) return ESP_FAIL;
    content[ret] = '\0';
    JsonDocument doc;
    if (deserializeJson(doc, content)) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON"); return ESP_FAIL; }

    bool wifi_changed = false;

    if (doc.containsKey("servoCenterDeg")) nvs_put_u32("servoCenterDeg", doc["servoCenterDeg"]);
    if (doc.containsKey("servoLimitLDeg")) nvs_put_u32("servoLimitLDeg", doc["servoLimitLDeg"]);
    if (doc.containsKey("servoLimitRDeg")) nvs_put_u32("servoLimitRDeg", doc["servoLimitRDeg"]);
    if (doc.containsKey("motorMaxSpeed"))  nvs_put_u32("motorMaxSpeed",  doc["motorMaxSpeed"]);
    if (doc.containsKey("motorMinSpeed"))  nvs_put_u32("motorMinSpeed",  doc["motorMinSpeed"]);
    if (doc.containsKey("enableScan"))     nvs_put_u32("enableScan",     doc["enableScan"]);
    if (doc.containsKey("autoTurnSignals"))nvs_put_bool("autoTurnSignals", (bool)doc["autoTurnSignals"]);
    if (doc.containsKey("autoTurnTol"))    nvs_put_u32("autoTurnTol",    doc["autoTurnTol"]);
    if (doc.containsKey("ledConfig"))      nvs_put_str("ledConfig",      doc["ledConfig"].as<const char*>());
    if (doc.containsKey("program"))        g_programManager->loadProgram(doc["program"].as<JsonArray>());

    auto check_wifi = [&](const char* key) {
        if (!doc.containsKey(key)) return;
        const char* new_val = doc[key].as<const char*>();
        if (!nvs_str_equals(key, new_val)) {
            nvs_put_str(key, new_val);
            wifi_changed = true;
        }
    };
    check_wifi("wifiName");
    check_wifi("wifiPass");
    check_wifi("wifiMode");
    if (doc.containsKey("camIP")) nvs_put_str("camIP", doc["camIP"].as<const char*>());

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);

    if (wifi_changed) { vTaskDelay(1000 / portTICK_PERIOD_MS); esp_restart(); }
    return ESP_OK;
}

// ---- Sequence (kids mode) ----

void sequenceTask(void *pvParameters) {
    isSequenceRunning = true;
    int iterations = sequenceIterations;
    bool infinite = (iterations == -1);

    for (int i = 0; infinite || i < iterations; i++) {
        if (!isSequenceRunning) break;
        JsonArray commands = sequenceCommands["commands"].as<JsonArray>();
        for (JsonVariant command : commands) {
            if (!isSequenceRunning) break;
            const char* cmd_str = command.as<const char*>();
            if (strcmp(cmd_str, "forward") == 0)       { setSteer(0, g_state); setMotor(1024, true, g_state); vTaskDelay(1000 / portTICK_PERIOD_MS); }
            else if (strcmp(cmd_str, "backward") == 0) { setSteer(0, g_state); setMotor(1024, false, g_state); vTaskDelay(1000 / portTICK_PERIOD_MS); }
            else if (strcmp(cmd_str, "left") == 0)     { setSteer(-512, g_state); setMotor(1024, true, g_state); vTaskDelay(1000 / portTICK_PERIOD_MS); }
            else if (strcmp(cmd_str, "right") == 0)    { setSteer(512, g_state); setMotor(1024, true, g_state); vTaskDelay(1000 / portTICK_PERIOD_MS); }
            else if (strcmp(cmd_str, "forward-left") == 0)  { setMotor(1024, true, g_state); setSteer(-512, g_state); vTaskDelay(1000 / portTICK_PERIOD_MS); }
            else if (strcmp(cmd_str, "forward-right") == 0) { setMotor(1024, true, g_state); setSteer(512, g_state); vTaskDelay(1000 / portTICK_PERIOD_MS); }
            else if (strcmp(cmd_str, "backward-left") == 0) { setMotor(1024, false, g_state); setSteer(-512, g_state); vTaskDelay(1000 / portTICK_PERIOD_MS); }
            else if (strcmp(cmd_str, "backward-right") == 0){ setMotor(1024, false, g_state); setSteer(512, g_state); vTaskDelay(1000 / portTICK_PERIOD_MS); }
            else if (strcmp(cmd_str, "wait") == 0)     { setMotor(0, false, g_state); setSteer(0, g_state); vTaskDelay(1000 / portTICK_PERIOD_MS); }
            if (!isSequenceRunning) break;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
    }
    setMotor(0, false, g_state);
    setSteer(0, g_state);
    isSequenceRunning = false;
    sequenceTaskHandle = NULL;
    vTaskDelete(NULL);
}

static esp_err_t get_sequence_stop_handler(httpd_req_t *req) {
    isSequenceRunning = false;
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t post_sequence_handler(httpd_req_t *req) {
    if (isSequenceRunning) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Sequence already running."); return ESP_FAIL; }
    char content[MAX_POST_SIZE];
    size_t recv_size = MIN(req->content_len, MAX_POST_SIZE);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) { if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req); return ESP_FAIL; }
    content[ret] = '\0';
    if (deserializeJson(sequenceCommands, content)) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON"); return ESP_FAIL; }
    sequenceIterations = sequenceCommands["iterations"] | 1;
    xTaskCreate(sequenceTask, "Sequence Task", 4096, NULL, 5, &sequenceTaskHandle);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

// ---- OTA ----

#define OTA_RECV_BUF_SIZE 4096

static esp_err_t get_ota_info_handler(httpd_req_t *req) {
    JsonDocument doc;
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* boot    = esp_ota_get_boot_partition();
    const esp_partition_t* next    = esp_ota_get_next_update_partition(NULL);
    if (running) { doc["running"]["label"] = running->label; doc["running"]["address"] = running->address; doc["running"]["size"] = running->size; }
    if (boot)    { doc["boot"]["label"] = boot->label; doc["boot"]["address"] = boot->address; }
    if (next)    { doc["next"]["label"] = next->label; doc["next"]["address"] = next->address; doc["next"]["size"] = next->size; }
    const esp_app_desc_t* app_desc = esp_app_get_description();
    if (app_desc) {
        doc["app"]["version"]      = app_desc->version;
        doc["app"]["project_name"] = app_desc->project_name;
        doc["app"]["compile_date"] = app_desc->date;
        doc["app"]["compile_time"] = app_desc->time;
        doc["app"]["idf_version"]  = app_desc->idf_ver;
    }
    std::string output;
    serializeJson(doc, output);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, output.c_str(), output.length());
    return ESP_OK;
}

static esp_err_t post_ota_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "OTA: Iniciando. Tamaño esperado: %d bytes", req->content_len);
    if (req->content_len <= 0) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content-Length requerido"); return ESP_FAIL; }

    if (g_programManager) { if (g_programManager->isRunning()) g_programManager->stopProgram(); if (g_programManager->isRecording()) g_programManager->stopRecording(); }
    isSequenceRunning = false;
    if (g_state) { stopMotors(g_state); g_state->apiActEnabled = false; }

    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition"); return ESP_FAIL; }
    if ((size_t)req->content_len > update_partition->size) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Image too large"); return ESP_FAIL; }

    esp_ota_handle_t ota_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err)); return ESP_FAIL; }

    char* buf = (char*)malloc(OTA_RECV_BUF_SIZE);
    if (!buf) { esp_ota_abort(ota_handle); httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM"); return ESP_FAIL; }

    int total_received = 0, remaining = req->content_len;
    bool header_checked = false;

    while (remaining > 0) {
        int to_read = remaining < OTA_RECV_BUF_SIZE ? remaining : OTA_RECV_BUF_SIZE;
        int recv_len = httpd_req_recv(req, buf, to_read);
        if (recv_len <= 0) { if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) continue; esp_ota_abort(ota_handle); free(buf); httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Recv failed"); return ESP_FAIL; }
        if (!header_checked) {
            if ((uint8_t)buf[0] != 0xE9) { esp_ota_abort(ota_handle); free(buf); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Not a valid ESP-IDF binary"); return ESP_FAIL; }
            header_checked = true;
        }
        err = esp_ota_write(ota_handle, buf, recv_len);
        if (err != ESP_OK) { esp_ota_abort(ota_handle); free(buf); httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err)); return ESP_FAIL; }
        total_received += recv_len;
        remaining -= recv_len;
        if ((total_received & 0xFFFF) == 0) ESP_LOGI(TAG, "OTA progreso: %d / %d bytes", total_received, req->content_len);
    }
    free(buf);

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) { httpd_resp_send_err(req, err == ESP_ERR_OTA_VALIDATE_FAILED ? HTTPD_400_BAD_REQUEST : HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err)); return ESP_FAIL; }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) { httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err)); return ESP_FAIL; }

    ESP_LOGI(TAG, "OTA completada (%d bytes). Reiniciando...", total_received);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"OTA OK. Reiniciando.\"}", -1);
    vTaskDelay(1500 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}
