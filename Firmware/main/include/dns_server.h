#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Starts a minimal captive-portal DNS server on UDP port 53.
// All queries are answered with `ip` (dotted decimal string).
// Call once after WiFi AP is up. The server runs in its own FreeRTOS task.
void dns_server_start(const char* ip);
void dns_server_stop(void);

#ifdef __cplusplus
}
#endif
