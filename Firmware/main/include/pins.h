#pragma once
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "sdkconfig.h"

// LEDC channel assignments are fixed (only GPIO numbers are runtime-configurable).
#define STEER_SERVO_CHANNEL  LEDC_CHANNEL_2
#define PAN_SERVO_CHANNEL    LEDC_CHANNEL_3
#define TILT_SERVO_CHANNEL   LEDC_CHANNEL_4
#define ESC_CHANNEL          LEDC_CHANNEL_5  // Señal del ESC brushless (reusa pinMotorEn)

// Compile-time defaults — used as NVS fallback and for "reset to defaults".
#if defined(CONFIG_IDF_TARGET_ESP32C6)
  #define PIN_CHIP_TYPE        "esp32c6"
  #define PIN_LED_STRIP_DEFAULT   8
  #define PIN_MOTOR_EN_DEFAULT   19
  #define PIN_MOTOR_DIR1_DEFAULT 20
  #define PIN_MOTOR_DIR2_DEFAULT 21
  #define PIN_STEER_DEFAULT       7
  #define PIN_PAN_DEFAULT         4
  #define PIN_TILT_DEFAULT        5
#elif defined(CONFIG_IDF_TARGET_ESP32)
  #define PIN_CHIP_TYPE        "esp32"
  #define PIN_LED_STRIP_DEFAULT  16
  #define PIN_MOTOR_EN_DEFAULT   25
  #define PIN_MOTOR_DIR1_DEFAULT 32
  #define PIN_MOTOR_DIR2_DEFAULT 33
  #define PIN_STEER_DEFAULT      13
  #define PIN_PAN_DEFAULT        14
  #define PIN_TILT_DEFAULT       27
#else
  #error "Unsupported target. Please define pin defaults for your board in pins.h"
#endif
