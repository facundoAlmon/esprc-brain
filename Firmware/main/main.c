// SPDX-License-Identifier: Apache-2.0

#include "sdkconfig.h"
#include <stddef.h>

// BTstack
#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <btstack_stdio_esp32.h>

// Bluepad32
#include <uni.h>

// Our custom platform (replaces bluepad32_arduino's arduino_platform)
#include "bp32_platform.h"

// Declared in sketch.cpp — starts the FreeRTOS main_task after BT init.
void app_task_start(void);

int app_main(void) {
#ifndef CONFIG_ESP_CONSOLE_UART_NONE
#ifndef CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
    btstack_stdio_init();
#endif
#endif

    // Configure BTstack for ESP32 VHCI Controller
    btstack_init();

    // Register our custom platform
    uni_platform_set_custom(get_bp32_platform());

    // Init Bluepad32 (internally creates BT tasks)
    uni_init(0, NULL);

    // Start our application task (WiFi, web server, actuators, main loop)
    app_task_start();

    // Does not return — BTstack run loop takes over
    btstack_run_loop_execute();

    return 0;
}
