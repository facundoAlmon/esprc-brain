/**
 * @file actuators.cpp
 * @brief Implementación del control de actuadores (motor y servo).
 */
#include "actuators.h"
#include "esp_log.h"
#include "driver/mcpwm.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "soc/mcpwm_periph.h"
#include "pins.h"

#define SERVO_FREQ_HZ    50
#define SERVO_MIN_US     500
#define SERVO_MAX_US     2500
#define SERVO_MAX_ANGLE  180
#define SERVO_RES_BITS   LEDC_TIMER_14_BIT
#define SERVO_RES_MAX    ((1u << 14) - 1)
#define SERVO_PERIOD_US  (1000000 / SERVO_FREQ_HZ)

static void servo_write_angle(int degrees) {
    uint32_t pulse_us = SERVO_MIN_US +
        (uint32_t)((uint32_t)degrees * (SERVO_MAX_US - SERVO_MIN_US) / SERVO_MAX_ANGLE);
    uint32_t duty = (uint32_t)((uint64_t)pulse_us * SERVO_RES_MAX / SERVO_PERIOD_US);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, servoChannel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, servoChannel);
}

/**
 * @brief Configura e inicializa los pines y periféricos para los actuadores.
 */
void setupActuators() {
    // Configura el periférico MCPWM para el control del motor.
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, (gpio_num_t)enable1Pin);
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 20000;    // Frecuencia de PWM de 20kHz, común para ESCs.
    pwm_config.cmpr_a = 0;           // Ciclo de trabajo inicial 0%.
    pwm_config.cmpr_b = 0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    // Configura los pines de dirección del motor.
    gpio_set_direction((gpio_num_t)motor1Pin1, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)motor1Pin2, GPIO_MODE_OUTPUT);

    // Configura el servo de dirección vía LEDC directo.
    ledc_timer_config_t servo_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = SERVO_RES_BITS,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = SERVO_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&servo_timer);
    ledc_channel_config_t servo_ch = {
        .gpio_num   = servoPin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = servoChannel,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&servo_ch);
}

/**
 * @brief Establece la velocidad y dirección del motor.
 */
void setMotor(int speed, bool forward, VehicleState* state) {
    // Lógica de luces de freno y reversa
    if (!forward && speed > 0) {
        // Si el motor va en reversa, encender la luz de reversa.
        state->reverseLedOn = true;
        state->brakesLedOn = false;
    } else if (state->lastMotorForward && !forward && speed > 0) {
        // Si el motor estaba yendo hacia adelante y ahora se le da marcha atrás (frenando).
        state->reverseLedOn = false;
        state->brakesLedOn = true;
    } else {
        // En cualquier otro caso, ambas luces apagadas.
        state->reverseLedOn = false;
        state->brakesLedOn = false;
    }
    state->lastMotorSpeed = speed;
    state->lastMotorForward = forward;

    float dutyCycle = 0;
    // Calcula el rango dinámico de velocidad.
    unsigned int motorMult = state->motorMaxSpeed - state->motorMinSpeed;

    if (speed != 0) {
        // Establece la dirección del motor.
        if (forward) {
            gpio_set_level((gpio_num_t)motor1Pin1, 0);
            gpio_set_level((gpio_num_t)motor1Pin2, 1);
        } else {
            gpio_set_level((gpio_num_t)motor1Pin1, 1);
            gpio_set_level((gpio_num_t)motor1Pin2, 0);
        }
        // Mapea la velocidad (0-1023) al rango de PWM configurado.
        dutyCycle = ((abs(speed) * motorMult) / 1024.0) + state->motorMinSpeed;
        ESP_LOGI("DC", "Motor speed: %d, Duty cycle: %.2f%%", speed, dutyCycle * 100.0 / 1024.0);
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, dutyCycle * 100.0 / 1024.0);
        mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    } else {
        // Detiene el motor.
        gpio_set_level((gpio_num_t)motor1Pin1, 0);
        gpio_set_level((gpio_num_t)motor1Pin2, 0);
    }
}

/**
 * @brief Establece el ángulo de la dirección.
 */
void setSteer(int angle, VehicleState* state) {
    float porc = (float)angle / 512.0; // Normaliza el ángulo a un rango de -1.0 a 1.0.
    int posDegrees = 0;

    int threshold = (state->autoTurnTol / 100.0) * 512.0;

    int currentSteerDirection = 0;
    if (angle > threshold) {
        currentSteerDirection = 1; // Derecha
    } else if (angle < -threshold) {
        currentSteerDirection = -1; // Izquierda
    }

    // Lógica de intermitentes automáticos para ENCENDIDO
    if (state->autoTurnSignals && currentSteerDirection != 0 && currentSteerDirection != state->lastSteerDirection) {
        if (currentSteerDirection == 1) {
            state->giroDerecho = true;
            state->giroIzquierdo = false;
        } else if (currentSteerDirection == -1) {
            state->giroIzquierdo = true;
            state->giroDerecho = false;
        }
    }

    // Lógica para apagar intermitentes al volver al centro
    if (currentSteerDirection == 0 && state->lastSteerDirection != 0) {
        if (state->lastSteerDirection == 1) {
            state->giroDerecho = false;
        } else if (state->lastSteerDirection == -1) {
            state->giroIzquierdo = false;
        }
    }

    state->lastSteerDirection = currentSteerDirection;

    if (angle == 0) {
        posDegrees = state->servoCenterDeg;
    } else if (porc < 0) { // Izquierda
        float movS = (state->servoLimitLDeg * porc);
        posDegrees = state->servoCenterDeg + movS;
    } else { // Derecha
        float movS = (state->servoLimitRDeg * porc);
        posDegrees = state->servoCenterDeg + movS;
    }
    servo_write_angle(posDegrees);
}

/**
 * @brief Detiene todos los actuadores.
 */
void stopMotors(VehicleState* state) {
    gpio_set_level((gpio_num_t)motor1Pin1, 0);
    gpio_set_level((gpio_num_t)motor1Pin2, 0);
    servo_write_angle(state->servoCenterDeg);
}
