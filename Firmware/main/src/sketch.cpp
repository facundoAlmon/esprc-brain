/**
 * @file sketch.cpp
 * @brief Archivo principal del firmware del ESP32.
 * 
 * Este archivo contiene las funciones `setup()` y `loop()`, que son el punto de
 * entrada del programa. Se encarga de inicializar todos los subsistemas
 * (WiFi, actuadores, gamepad, servidor web, etc.) y de ejecutar el bucle
 * principal que mantiene el vehículo en funcionamiento.
 */
#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_coexist.h"

#include "state.h"
#include "actuators.h"
#include "gamepadHandler.h"
#include "ledStripHandler.h"
#include "webServerHandler.h"
#include "ProgramManager.h"

#include "pins.h"

// Instancia global de la estructura de estado del vehículo.
VehicleState vehicleState;
ProgramManager programManager(&vehicleState);

// Objeto para manejar el almacenamiento no volátil (NVS).
Preferences preferences;

// Variables para el bucle no bloqueante.
unsigned long lastLoopTime = 0;
const unsigned long loopInterval = 15; // ms

// --- Configuración y manejo de WiFi ---
static const char *TAG = "WIFI";
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;

static void start_ap_mode(bool is_fallback); // Declaración adelantada

/**
 * @brief Manejador de eventos de WiFi. 
 * 
 * Gestiona eventos como la conexión, desconexión y obtención de IP.
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data)
{
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
        ESP_LOGI(TAG,"Fallo al conectar al AP");
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

/**
 * @brief Inicia el modo Access Point (AP).
 * 
 * @param is_fallback Si es true, crea una red abierta de respaldo.
 *                    Si es false, usa la configuración guardada.
 * En ambos casos, aplica el workaround de coexistencia para BT/WiFi.
 */
static void start_ap_mode(bool is_fallback) {
    if (is_fallback) {
        ESP_LOGI(TAG, "Iniciando en modo Access Point (AP) de respaldo");
    } else {
        ESP_LOGI(TAG, "Iniciando en modo Access Point (AP)");
    }

    // Workaround de coexistencia: Deshabilita el escaneo BT para fiabilidad del AP.
    vehicleState.enableScan = 0;

    esp_netif_t* ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {};
    
    if (is_fallback) {
        strncpy((char*)wifi_config.ap.ssid, "ESP-RC-CAR", sizeof(wifi_config.ap.ssid));
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
    } else { // Modo AP primario
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
    if (is_fallback) {
        ESP_LOGI(TAG, "Modo AP de respaldo iniciado. IP: %s", vehicleState.espIP);
    } else {
        ESP_LOGI(TAG, "Modo AP iniciado. IP: %s", vehicleState.espIP);
    }
}

/**
 * @brief Inicia el modo Cliente (STA) y cambia a modo AP si falla.
 */
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
        Console.printf("\nWiFi Conectado. IP: %s\n", vehicleState.espIP);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "Fallo al conectar a %s. Cambiando a modo AP de respaldo.", vehicleState.wifiName);
        // Limpia recursos de STA y vuelve a iniciar en modo AP.
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(sta_netif);
        vEventGroupDelete(s_wifi_event_group);

        start_ap_mode(true); // Llamada explícita al modo de respaldo
    } else {
        ESP_LOGE(TAG, "EVENTO INESPERADO");
    }
}


/**
 * @brief Inicializa el WiFi en modo Access Point (AP) o Cliente (STA).
 */
void initWiFi() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (strcmp(vehicleState.wifiMode, "AP") == 0) {
        start_ap_mode(false); // Llamada explícita al modo primario
    } else { // Modo Cliente (STA)
        start_sta_mode();
    }
    esp_wifi_set_ps(WIFI_PS_NONE);
}



#include <ArduinoJson.h>

// ... (resto de includes)

// ... (código anterior)

/**
 * @brief Carga la configuración guardada desde la memoria no volátil (NVS).
 */
void initPreferences() {
    preferences.begin("bl-car", false);

    // Carga configuración del vehículo.
    vehicleState.servoCenterDeg = preferences.getUInt("servoCenterDeg", 93);
    vehicleState.servoLimitLDeg = preferences.getUInt("servoLimitLDeg", 35);
    vehicleState.servoLimitRDeg = preferences.getUInt("servoLimitRDeg", 45);
    vehicleState.motorMinSpeed = preferences.getUInt("motorMinSpeed", 150);
    vehicleState.motorMaxSpeed = preferences.getUInt("motorMaxSpeed", 255);
    vehicleState.autoTurnSignals = preferences.getBool("autoTurnSignals", true);
    vehicleState.autoTurnTol = preferences.getUInt("autoTurnTol", 10);
    vehicleState.lastSteerDirection = 0;
    vehicleState.lastMotorSpeed = 0;
    vehicleState.lastMotorForward = true;
    vehicleState.enableScan = preferences.getUInt("enableScan", 0);
    
    // Carga configuración de WiFi. Si no existe una clave, se crea con valores por defecto
    // para evitar errores de "NOT_FOUND" en el log en cada arranque.
    if (preferences.isKey("wifiName")) {
        preferences.getString("wifiName", vehicleState.wifiName, sizeof(vehicleState.wifiName));
    } else {
        strncpy(vehicleState.wifiName, "ESP-RC-CAR", sizeof(vehicleState.wifiName));
        preferences.putString("wifiName", vehicleState.wifiName);
    }
    if (preferences.isKey("wifiPass")) {
        preferences.getString("wifiPass", vehicleState.wifiPass, sizeof(vehicleState.wifiPass));
    } else {
        strncpy(vehicleState.wifiPass, "", sizeof(vehicleState.wifiPass));
        preferences.putString("wifiPass", vehicleState.wifiPass);
    }
    if (preferences.isKey("wifiMode")) {
        preferences.getString("wifiMode", vehicleState.wifiMode, sizeof(vehicleState.wifiMode));
    } else {
        strncpy(vehicleState.wifiMode, "AP", sizeof(vehicleState.wifiMode));
        preferences.putString("wifiMode", vehicleState.wifiMode);
    }

    // Workaround de coexistencia: Si se inicia en modo AP, deshabilita el escaneo BT
    // para asegurar que el AP de WiFi se inicie de forma fiable.
    if (strcmp(vehicleState.wifiMode, "AP") == 0) {
        vehicleState.enableScan = 0;
    }

    vehicleState.apiActMSTimeout = 30000;

    // Carga configuración de LEDs desde JSON. Si no existe, la crea con valores por defecto.
    String ledConfigJson;
    if (preferences.isKey("ledConfig")) {
        ledConfigJson = preferences.getString("ledConfig");
    } else {
        ledConfigJson = "{}";
        preferences.putString("ledConfig", ledConfigJson);
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, ledConfigJson);

    if (error) {
        ESP_LOGE(TAG, "Fallo al parsear la configuración de LEDs: %s", error.c_str());
        // Cargar una configuración por defecto si falla el parseo
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

/**
 * @brief Función de configuración principal, se ejecuta una vez al arrancar.
 */
void setup() {
    Console.printf("Firmware: %s\n", BP32.firmwareVersion());
    
    initPreferences();
    programManager.loadProgramFromNVS(); // Carga el programa guardado en NVS al arrancar
    setupLedStrip(&vehicleState); // This calls configure_led() 

    led_strip_set_pixel(led_strip, 0, 255, 0, 0); // LED rojo: iniciando
    led_strip_refresh(led_strip);

    initWiFi();
    Console.printf("WiFi: %s (%s)\n", vehicleState.wifiName, vehicleState.wifiMode);
    setupActuators();

    led_strip_set_pixel(led_strip, 0, 0, 0, 255); // LED azul: WiFi/actuadores listos
    led_strip_refresh(led_strip);

    setupGamepad(&vehicleState, &programManager); // This calls BP32.setup() 

    led_strip_set_pixel(led_strip, 0, 0, 255, 0); // LED verde: todo listo
    led_strip_refresh(led_strip);

    startServer(&vehicleState, &programManager);
    
    led_strip_set_pixel(led_strip, 0, 0, 0, 0); // Apaga el LED de estado
    led_strip_refresh(led_strip);
}

/**
 * @brief Bucle principal del programa, se ejecuta repetidamente.
 */
void loop() {
    unsigned long currentTime = millis();

    // Bucle no bloqueante con un intervalo fijo.
    if (currentTime - lastLoopTime >= loopInterval) {
        lastLoopTime = currentTime;

        // Solo procesa el gamepad si no hay un programa en ejecución
        if (!programManager.isRunning()) {
            handleGamepads(&vehicleState, &programManager);
        }

        // Timeout para las acciones recibidas por la API.
        if (vehicleState.apiActEnabled) {
            if ((millis() - vehicleState.apiActMSStart) >= vehicleState.apiActMS) {
                stopMotors(&vehicleState);
                vehicleState.apiActEnabled = false;
                vehicleState.apiActMSStart = 0;
                vehicleState.apiActMS = 0;
            }
        }
        
        programManager.loop(); // Ejecuta el bucle del gestor de programas
        handleLedStrip(&vehicleState, &programManager);
    }
}
