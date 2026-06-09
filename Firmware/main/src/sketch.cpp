#include <string.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_coexist.h"
#include "esp_ota_ops.h"
#include <ArduinoJson.h>

#include "state.h"
#include "actuators.h"
#include "gamepadHandler.h"
#include "ledStripHandler.h"
#include "webServerHandler.h"
#include "ProgramManager.h"
#include "nvs_prefs.h"
#include "pins.h"
#include "espnow_manager.h"

static inline uint32_t millis() { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

VehicleState vehicleState;
ProgramManager programManager(&vehicleState);
NvsPrefs preferences;

static const char *TAG = "WIFI";
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;

static void start_ap_mode(bool is_fallback);

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Reintentando conectar al AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "Fallo al conectar al AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP obtenida:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Estación " MACSTR " conectada, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Estación " MACSTR " desconectada, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

static void start_ap_mode(bool is_fallback) {
    ESP_LOGI(TAG, "Iniciando en modo Access Point (AP)%s", is_fallback ? " de respaldo" : "");

    // Workaround de coexistencia: deshabilita el escaneo BT en AP.
    vehicleState.enableScan = 0;

    esp_netif_t* ap_netif = esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {};
    if (is_fallback) {
        strncpy((char*)wifi_config.ap.ssid, "ESP-RC-CAR", sizeof(wifi_config.ap.ssid));
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        strncpy((char*)wifi_config.ap.ssid, vehicleState.wifiName, sizeof(wifi_config.ap.ssid));
        strncpy((char*)wifi_config.ap.password, vehicleState.wifiPass, sizeof(wifi_config.ap.password));
        wifi_config.ap.authmode = (strlen(vehicleState.wifiPass) < 8) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    }
    wifi_config.ap.ssid_len = strlen((char*)wifi_config.ap.ssid);
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_coex_preference_set(ESP_COEX_PREFER_WIFI));

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(ap_netif, &ip_info);
    esp_ip4addr_ntoa(&ip_info.ip, vehicleState.espIP, sizeof(vehicleState.espIP));
    strncpy(vehicleState.wifiMode, "AP", sizeof(vehicleState.wifiMode));
    ESP_LOGI(TAG, "Modo AP%s iniciado. IP: %s", is_fallback ? " de respaldo" : "", vehicleState.espIP);
}

static void start_sta_mode() {
    ESP_LOGI(TAG, "Iniciando en modo Cliente (STA)");
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id, instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, vehicleState.wifiName, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, vehicleState.wifiPass, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(sta_netif, &ip_info);
        esp_ip4addr_ntoa(&ip_info.ip, vehicleState.espIP, sizeof(vehicleState.espIP));
        ESP_LOGI(TAG, "WiFi Conectado. IP: %s", vehicleState.espIP);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "Fallo al conectar a %s. Cambiando a AP de respaldo.", vehicleState.wifiName);
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(sta_netif);
        vEventGroupDelete(s_wifi_event_group);
        start_ap_mode(true);
    }
}

void initWiFi() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (strcmp(vehicleState.wifiMode, "AP") == 0) {
        start_ap_mode(false);
    } else {
        start_sta_mode();
    }
    esp_wifi_set_ps(WIFI_PS_NONE);
}

void initPreferences() {
    preferences.begin("bl-car", false);

    vehicleState.servoCenterDeg  = preferences.getUInt("servoCenterDeg", 93);
    vehicleState.servoLimitLDeg  = preferences.getUInt("servoLimitLDeg", 35);
    vehicleState.servoLimitRDeg  = preferences.getUInt("servoLimitRDeg", 45);
    vehicleState.motorMinSpeed   = preferences.getUInt("motorMinSpeed", 150);
    vehicleState.motorMaxSpeed   = preferences.getUInt("motorMaxSpeed", 255);
    vehicleState.autoTurnSignals = preferences.getBool("autoTurnSignals", true);
    vehicleState.autoTurnTol     = preferences.getUInt("autoTurnTol", 10);
    vehicleState.lastSteerDirection = 0;
    vehicleState.lastMotorSpeed     = 0;
    vehicleState.lastMotorForward   = true;
    vehicleState.enableScan = preferences.getUInt("enableScan", 0);

    if (preferences.isKey("wifiName")) {
        std::string v = preferences.getString("wifiName");
        strncpy(vehicleState.wifiName, v.c_str(), sizeof(vehicleState.wifiName));
    } else {
        strncpy(vehicleState.wifiName, "ESP-RC-CAR", sizeof(vehicleState.wifiName));
        preferences.putString("wifiName", vehicleState.wifiName);
    }
    if (preferences.isKey("wifiPass")) {
        std::string v = preferences.getString("wifiPass");
        strncpy(vehicleState.wifiPass, v.c_str(), sizeof(vehicleState.wifiPass));
    } else {
        strncpy(vehicleState.wifiPass, "", sizeof(vehicleState.wifiPass));
        preferences.putString("wifiPass", vehicleState.wifiPass);
    }
    if (preferences.isKey("wifiMode")) {
        std::string v = preferences.getString("wifiMode");
        strncpy(vehicleState.wifiMode, v.c_str(), sizeof(vehicleState.wifiMode));
    } else {
        strncpy(vehicleState.wifiMode, "AP", sizeof(vehicleState.wifiMode));
        preferences.putString("wifiMode", vehicleState.wifiMode);
    }

    if (strcmp(vehicleState.wifiMode, "AP") == 0) {
        vehicleState.enableScan = 0;
    }

    vehicleState.apiActMSTimeout = 30000;

    // Camera servos
    vehicleState.camServoEnabled   = preferences.getBool("camServoEn",   false);
    vehicleState.camGamepadEnabled = preferences.getBool("camGamepadEn", false);
    vehicleState.panInvert         = preferences.getBool("panInvert",    false);
    vehicleState.tiltInvert        = preferences.getBool("tiltInvert",   false);
    vehicleState.camStickDZ        = preferences.getUInt("camStickDZ",   30);
    vehicleState.camStickSat       = preferences.getUInt("camStickSat",  350);
    vehicleState.panCenterDeg      = preferences.getUInt("panCenterDeg",  90);
    vehicleState.panLimitLDeg      = preferences.getUInt("panLimitLDeg",  45);
    vehicleState.panLimitRDeg      = preferences.getUInt("panLimitRDeg",  45);
    vehicleState.panMinUs          = preferences.getUInt("panMinUs",      500);
    vehicleState.panMaxUs          = preferences.getUInt("panMaxUs",      2400);
    vehicleState.tiltCenterDeg     = preferences.getUInt("tiltCenterDeg", 90);
    vehicleState.tiltLimitUpDeg    = preferences.getUInt("tiltLimUpDeg",  30);
    vehicleState.tiltLimitDownDeg  = preferences.getUInt("tiltLimDnDeg",  30);
    vehicleState.tiltMinUs         = preferences.getUInt("tiltMinUs",     500);
    vehicleState.tiltMaxUs         = preferences.getUInt("tiltMaxUs",     2400);
    vehicleState.lastPanAngle      = 0;
    vehicleState.lastTiltAngle     = 0;

    // LED config
    std::string ledConfigJson;
    if (preferences.isKey("ledConfig")) {
        ledConfigJson = preferences.getString("ledConfig");
    } else {
        ledConfigJson = "{}";
        preferences.putString("ledConfig", "{}");
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, ledConfigJson);
    if (error) {
        ESP_LOGE(TAG, "Fallo al parsear ledConfig: %s", error.c_str());
        vehicleState.ledCount = 12;
    } else {
        vehicleState.ledCount = doc["total_leds"] | 12;
        JsonArray groups = doc["grupos"].as<JsonArray>();
        vehicleState.ledGroups.clear();
        for (JsonObject group : groups) {
            LedGroup newGroup;
            newGroup.funcion = (LedFunction)group["funcion"].as<int>();
            strncpy(newGroup.leds, group["leds"].as<const char*>(), sizeof(newGroup.leds));
            newGroup.colorR = group["color"]["r"];
            newGroup.colorG = group["color"]["g"];
            newGroup.colorB = group["color"]["b"];
            newGroup.brillo = group["brillo"];
            vehicleState.ledGroups.push_back(newGroup);
        }
    }
}

// Main application task — equivalent to Arduino setup() + loop()
static void main_task(void* pvParameters) {
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);

    initPreferences();
    programManager.loadProgramFromNVS();
    setupLedStrip(&vehicleState);

    led_strip_set_pixel(led_strip, 0, 255, 0, 0);
    led_strip_refresh(led_strip);

    initWiFi();
    ESP_LOGI(TAG, "WiFi: %s (%s)", vehicleState.wifiName, vehicleState.wifiMode);

    wifi_interface_t espnow_iface = (strcmp(vehicleState.wifiMode, "AP") == 0) ? WIFI_IF_AP : WIFI_IF_STA;
    espnow_init(&vehicleState, espnow_iface);

    setupActuators();
    setupCamServos(&vehicleState);

    led_strip_set_pixel(led_strip, 0, 0, 0, 255);
    led_strip_refresh(led_strip);

    setupGamepad(&vehicleState, &programManager);

    led_strip_set_pixel(led_strip, 0, 0, 255, 0);
    led_strip_refresh(led_strip);

    startServer(&vehicleState, &programManager);

    led_strip_set_pixel(led_strip, 0, 0, 0, 0);
    led_strip_refresh(led_strip);

    // Mark OTA image as valid if needed
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (running && esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "OTA PENDING_VERIFY: marcando como valida");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }

    // Main loop (15 ms non-blocking interval)
    uint32_t lastLoopTime = 0;
    const uint32_t loopInterval = 15;

    for (;;) {
        uint32_t currentTime = millis();
        if (currentTime - lastLoopTime >= loopInterval) {
            lastLoopTime = currentTime;

            handleGamepadButtons(&vehicleState, &programManager);

            if (!programManager.isRunning()) {
                handleGamepadMotion(&vehicleState);
            }

            if (vehicleState.apiActEnabled) {
                if ((millis() - vehicleState.apiActMSStart) >= vehicleState.apiActMS) {
                    stopMotors(&vehicleState);
                    vehicleState.apiActEnabled = false;
                    vehicleState.apiActMSStart = 0;
                    vehicleState.apiActMS = 0;
                }
            }

            programManager.loop();
            handleLedStrip(&vehicleState, &programManager);
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

// Called from main.c
extern "C" void app_task_start(void) {
    xTaskCreatePinnedToCore(main_task, "main_task", 8192, NULL, 5, NULL, tskNO_AFFINITY);
}
