// Minimal captive-portal DNS server for AP mode.
// Responds to every DNS query with the ESP32's own IP address.

#include "dns_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include <stdint.h>

#define DNS_PORT      53
#define DNS_BUF_SIZE  512

static const char* TAG = "DNSServer";
static TaskHandle_t s_task_handle = NULL;
static volatile bool s_running = false;
static char s_ip[16] = "192.168.4.1";

// DNS response: copies header, flips QR bit, appends an A-record answer.
static int build_response(const uint8_t* req, int req_len, uint8_t* resp, int resp_max, uint32_t ip_net) {
    if (req_len < 12) return -1;

    // Copy header
    memcpy(resp, req, 12);
    resp[2] = 0x81; // QR=1 (response), Opcode=0, AA=1
    resp[3] = 0x80; // RA=1, RCODE=0 (no error)
    resp[6] = 0;    // ANCOUNT high
    resp[7] = 1;    // ANCOUNT low = 1 answer
    resp[8] = 0;    // NSCOUNT = 0
    resp[9] = 0;
    resp[10] = 0;   // ARCOUNT = 0
    resp[11] = 0;

    // Copy question section verbatim
    int q_len = req_len - 12;
    if (12 + q_len + 16 > resp_max) return -1;
    memcpy(resp + 12, req + 12, q_len);
    int pos = 12 + q_len;

    // Answer: pointer to question name (0xC00C), TYPE A, CLASS IN, TTL 60, RDLEN 4, IP
    resp[pos++] = 0xC0; resp[pos++] = 0x0C; // name pointer
    resp[pos++] = 0x00; resp[pos++] = 0x01; // TYPE A
    resp[pos++] = 0x00; resp[pos++] = 0x01; // CLASS IN
    resp[pos++] = 0x00; resp[pos++] = 0x00; // TTL high
    resp[pos++] = 0x00; resp[pos++] = 60;   // TTL low (60 s)
    resp[pos++] = 0x00; resp[pos++] = 0x04; // RDLENGTH = 4
    memcpy(resp + pos, &ip_net, 4);
    pos += 4;

    return pos;
}

static void dns_task(void* arg) {
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(DNS_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) { ESP_LOGE(TAG, "socket() failed"); vTaskDelete(NULL); return; }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "bind() failed"); close(sock); vTaskDelete(NULL); return;
    }

    // Parse the IP string into network-order 32-bit int
    struct in_addr addr;
    inet_aton(s_ip, &addr);
    uint32_t ip_net = addr.s_addr;

    uint8_t req[DNS_BUF_SIZE], resp[DNS_BUF_SIZE];
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    ESP_LOGI(TAG, "DNS server started, redirecting to %s", s_ip);
    while (s_running) {
        int len = recvfrom(sock, req, sizeof(req), 0, (struct sockaddr*)&client, &client_len);
        if (len <= 0) continue; // timeout or error

        int resp_len = build_response(req, len, resp, sizeof(resp), ip_net);
        if (resp_len > 0) {
            sendto(sock, resp, resp_len, 0, (struct sockaddr*)&client, client_len);
        }
    }

    close(sock);
    ESP_LOGI(TAG, "DNS server stopped");
    vTaskDelete(NULL);
}

void dns_server_start(const char* ip) {
    if (s_running) return;
    strncpy(s_ip, ip, sizeof(s_ip) - 1);
    s_running = true;
    xTaskCreate(dns_task, "dns_server", 3072, NULL, 5, &s_task_handle);
}

void dns_server_stop(void) {
    s_running = false;
    s_task_handle = NULL;
}
