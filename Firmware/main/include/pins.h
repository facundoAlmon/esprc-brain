#pragma once
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "sdkconfig.h" // Needed for the target macros

#if defined(CONFIG_IDF_TARGET_ESP32C6)
// Pins for ESP32-C6
const int LED_STRIP_GPIO_PIN = 8;
const ledc_channel_t servoChannel = LEDC_CHANNEL_2;
const gpio_num_t servoPin = (gpio_num_t)7;
const int enable1Pin = 19;
const int motor1Pin1 = 20;
const int motor1Pin2 = 21;

#elif defined(CONFIG_IDF_TARGET_ESP32)
// Pins for ESP32 - Please define the correct pins here!
const int LED_STRIP_GPIO_PIN = 16;
const ledc_channel_t servoChannel = LEDC_CHANNEL_2;
const gpio_num_t servoPin = (gpio_num_t)13;
const int enable1Pin = 25;
const int motor1Pin1 = 32;
const int motor1Pin2 = 33;

#else
#error "Unsupported target. Please define the pins for your board in pins.h"
#endif
