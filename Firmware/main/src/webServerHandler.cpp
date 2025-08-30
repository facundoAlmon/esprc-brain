/**
 * @file webServerHandler.cpp
 * @brief Implementación del servidor web, API REST y WebSocket.
 */
#include "webServerHandler.h"
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "esp_http_server.h"
#include "actuators.h"
#include "state.h"
#include <string.h>

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

// Servidor DNS para el modo Access Point.
static DNSServer dnsServer;
const byte DNS_PORT = 53;

// Handle del servidor web.
static httpd_handle_t server_httpd = NULL;

// Puntero al estado global del vehículo.
static VehicleState* g_state = nullptr;
extern Preferences preferences;

// Declaraciones anticipadas de los manejadores de endpoints.
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

/**
 * @brief Genera una cadena JSON con la configuración actual del vehículo (sin LEDs).
 * @return char* Cadena JSON. El llamador debe liberar esta memoria con free().
 */
static char* get_config_json() {
    JsonDocument doc;
    doc["servoCenterDeg"] = g_state->servoCenterDeg;
    doc["servoLimitLDeg"] = g_state->servoLimitLDeg;
    doc["servoLimitRDeg"] = g_state->servoLimitRDeg;
    doc["motorMaxSpeed"] = g_state->motorMaxSpeed;
    doc["motorMinSpeed"] = g_state->motorMinSpeed;
    doc["enableScan"] = g_state->enableScan;

    char *json_string = (char*)malloc(512);
    serializeJson(doc, json_string, 512);
    return json_string;
}

/**
 * @brief Inicia el servidor web y registra todos los endpoints.
 */
void startServer(VehicleState* state) {
    g_state = state;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 15;
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.global_user_ctx = g_state;

    MDNS.begin("ecar");
    MDNS.addService("http", "tcp", 80);

    if (strcmp(g_state->wifiMode, "AP") == 0) {
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", g_state->espIP);
    }

    Console.printf("Iniciando servidor web en el puerto: '%d'\n", config.server_port);

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
    }
}

void stopServer() {
    if (server_httpd) {
        httpd_stop(server_httpd);
        server_httpd = NULL;
    }
}

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
    if (ret <= 0) { return ESP_FAIL; }
    content[ret] = '\0';

    JsonDocument doc;
    deserializeJson(doc, content);

    VehicleState* state = (VehicleState*)httpd_get_global_user_ctx(req->handle);
    state->servoCenterDeg = doc["servoCenterDeg"];
    state->servoLimitLDeg = doc["servoLimitLDeg"];
    state->servoLimitRDeg = doc["servoLimitRDeg"];
    state->motorMaxSpeed = doc["motorMaxSpeed"];
    state->motorMinSpeed = doc["motorMinSpeed"];
    state->enableScan = doc["enableScan"];

    BP32.enableNewBluetoothConnections(state->enableScan == 1);

    preferences.putUInt("servoCenterDeg", state->servoCenterDeg);
    preferences.putUInt("servoLimitLDeg", state->servoLimitLDeg);
    preferences.putUInt("servoLimitRDeg", state->servoLimitRDeg);
    preferences.putUInt("motorMaxSpeed", state->motorMaxSpeed);
    preferences.putUInt("motorMinSpeed", state->motorMinSpeed);
    preferences.putUInt("enableScan", state->enableScan);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t get_led_config_handler(httpd_req_t *req) {
    String ledConfigJson = preferences.getString("ledConfig", "{\"total_leds\": 12, \"grupos\": []}");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, ledConfigJson.c_str(), ledConfigJson.length());
    return ESP_OK;
}

static esp_err_t post_led_config_handler(httpd_req_t *req) {
    char content[MAX_POST_SIZE];
    size_t recv_size = MIN(req->content_len, MAX_POST_SIZE);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) { return ESP_FAIL; }
    content[ret] = '\0';

    preferences.putString("ledConfig", content);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);

    if (error) {
        ESP_LOGE("WebServer", "Fallo al parsear JSON de LEDs: %s", error.c_str());
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
    if (ret <= 0) { return ESP_FAIL; }
    content[ret] = '\0';

    JsonDocument doc;
    deserializeJson(doc, content);

    VehicleState* state = (VehicleState*)httpd_get_global_user_ctx(req->handle);
    strncpy(state->wifiName, doc["wifiName"], sizeof(state->wifiName));
    strncpy(state->wifiPass, doc["wifiPass"], sizeof(state->wifiPass));
    strncpy(state->wifiMode, doc["wifiMode"], sizeof(state->wifiMode));

    preferences.putString("wifiName", state->wifiName);
    preferences.putString("wifiPass", state->wifiPass);
    preferences.putString("wifiMode", state->wifiMode);

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
    if (ret <= 0) { return ESP_FAIL; }
    content[ret] = '\0';

    JsonDocument doc;
    deserializeJson(doc, content);

    if (doc["clearPreferences"]) {
        preferences.clear();
    }
    
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "OK", 2);

    if (doc["restartESP"]) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }
    
    return ESP_OK;
}

static void act_from_json(JsonDocument& doc, VehicleState* state) {
    bool has_actuator_cmd = doc.containsKey("motorSpeed") || doc.containsKey("motorDirection") || doc.containsKey("steerAng") || doc.containsKey("steerDirection");

    if (has_actuator_cmd) {
        state->apiActEnabled = true;
        state->apiActMSStart = millis();
        state->apiActMS = doc["ms"] | state->apiActMSTimeout;

        int motorSpeed = doc["motorSpeed"] | 0;
        const char* motorDirectionStr = doc["motorDirection"] | "F";
        setMotor(motorSpeed, strcmp(motorDirectionStr, "F") == 0, state);
        
        int steerAng = doc["steerAng"] | 0;
        const char* steerDirectionStr = doc["steerDirection"] | "R";
        int finalSteer = strcmp(steerDirectionStr, "L") == 0 ? -steerAng : steerAng;
        setSteer(finalSteer, state);
    }

    if (doc.containsKey("action")) {
        const char* action = doc["action"];
        if (strcmp(action, "headlights_cycle") == 0) {
            state->luces++;
            if (state->luces > 3) state->luces = 0;
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
    if (ret <= 0) { return ESP_FAIL; }
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
    if (req->method == HTTP_GET) {
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) { return ret; }

    if (ws_pkt.len) {
        buf = (uint8_t*)calloc(1, ws_pkt.len + 1);
        if (!buf) { return ESP_ERR_NO_MEM; }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            free(buf);
            return ret;
        }
        
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
