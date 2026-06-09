#include "espnow_manager.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "nvs.h"

static const char* TAG = "ESPNOW";

// ---- Protocol types ----
#define MSG_BEACON    0x01   // brain → broadcast
#define MSG_CAM_HELLO 0x02   // cam → brain

typedef struct __attribute__((packed)) {
    uint8_t type;       // MSG_BEACON
    char    ssid[32];
    char    pass[64];
    uint8_t channel;
} espnow_beacon_t;      // 98 bytes

typedef struct __attribute__((packed)) {
    uint8_t type;       // MSG_CAM_HELLO
    char    ip[16];     // empty if cam has no WiFi yet
} espnow_cam_hello_t;   // 17 bytes

// ---- Internal state ----
typedef struct {
    uint8_t mac[6];
    uint8_t data[250];
    int     len;
} recv_event_t;

static VehicleState*   s_state  = nullptr;
static wifi_interface_t s_iface = WIFI_IF_STA;
static QueueHandle_t   s_queue  = nullptr;

static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ---- Helpers ----
static void persist_cam_ip(const char* ip) {
    nvs_handle_t h;
    if (nvs_open("bl-car", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, "camIP", ip);
        nvs_commit(h);
        nvs_close(h);
    }
}

static void handle_cam_hello(const uint8_t* mac, const uint8_t* data, int len) {
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, mac, 6);
        peer.channel = 0;
        peer.ifidx   = s_iface;
        peer.encrypt = false;
        esp_now_add_peer(&peer);
        ESP_LOGI(TAG, "Cam peer registered " MACSTR, MAC2STR(mac));
    }

    if (len < (int)sizeof(espnow_cam_hello_t)) return;
    const espnow_cam_hello_t* msg = (const espnow_cam_hello_t*)data;

    char ip[16];
    strncpy(ip, msg->ip, sizeof(ip));
    ip[15] = '\0';

    if (ip[0] == '\0') return;
    if (strcmp(ip, s_state->cameraIP) == 0) return;

    strncpy(s_state->cameraIP, ip, sizeof(s_state->cameraIP));
    persist_cam_ip(ip);
    ESP_LOGI(TAG, "Camera IP updated via ESP-NOW: %s", ip);
}

// ---- Recv callback (runs in WiFi task — must be fast) ----
static void on_recv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    if (len < 1 || s_queue == nullptr) return;
    recv_event_t evt;
    memcpy(evt.mac, info->src_addr, 6);
    int copy_len = len < 250 ? len : 250;
    memcpy(evt.data, data, copy_len);
    evt.len = copy_len;
    xQueueSend(s_queue, &evt, 0);
}

// ---- Beacon + receive task ----
static void espnow_task(void*) {
    esp_now_peer_info_t bcast = {};
    memcpy(bcast.peer_addr, BROADCAST_MAC, 6);
    bcast.channel = 0;
    bcast.ifidx   = s_iface;
    bcast.encrypt = false;
    esp_now_add_peer(&bcast);

    recv_event_t evt;
    int64_t last_beacon_us = 0;
    // Fast while cam is undiscovered; slow once we have its IP (cam sends its
    // own 30s CAM_HELLO keepalive so beacons are only needed for reconnection).
    const int64_t BEACON_INTERVAL_FAST_US = 5000000LL;  // 5 s — discovery
    const int64_t BEACON_INTERVAL_SLOW_US = 30000000LL; // 30 s — maintenance

    for (;;) {
        int64_t now = esp_timer_get_time();
        bool cam_known = (s_state->cameraIP[0] != '\0');
        int64_t interval = cam_known ? BEACON_INTERVAL_SLOW_US : BEACON_INTERVAL_FAST_US;
        if (now - last_beacon_us >= interval) {
            last_beacon_us = now;

            espnow_beacon_t beacon = {};
            beacon.type = MSG_BEACON;
            strncpy(beacon.ssid, s_state->wifiName, sizeof(beacon.ssid));
            strncpy(beacon.pass, s_state->wifiPass, sizeof(beacon.pass));

            uint8_t primary = 0;
            wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
            esp_wifi_get_channel(&primary, &second);
            beacon.channel = primary;

            esp_now_send(BROADCAST_MAC, (uint8_t*)&beacon, sizeof(beacon));
            ESP_LOGD(TAG, "Beacon sent on channel %d", primary);
        }

        if (xQueueReceive(s_queue, &evt, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (evt.len < 1) continue;
            switch (evt.data[0]) {
                case MSG_CAM_HELLO:
                    handle_cam_hello(evt.mac, evt.data, evt.len);
                    break;
                default:
                    break;
            }
        }
    }
}

// ---- Public API ----
void espnow_init(VehicleState* state, wifi_interface_t iface) {
    s_state = state;
    s_iface = iface;
    s_queue = xQueueCreate(8, sizeof(recv_event_t));

    ESP_ERROR_CHECK(esp_now_init());
    esp_now_register_recv_cb(on_recv);

    xTaskCreate(espnow_task, "espnow", 4096, nullptr, 3, nullptr);
    ESP_LOGI(TAG, "ESP-NOW initialized (iface=%d)", iface);
}
