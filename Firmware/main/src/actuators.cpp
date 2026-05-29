/**
 * @file actuators.cpp
 * @brief Implementación del control de actuadores (motor y servo).
 */
#include "actuators.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "pins.h"

#define SERVO_FREQ_HZ    50
#define SERVO_MIN_US     500
#define SERVO_MAX_US     2500
#define SERVO_MAX_ANGLE  180
#define SERVO_RES_BITS   LEDC_TIMER_14_BIT
#define SERVO_RES_MAX    ((1u << 14) - 1)
#define SERVO_PERIOD_US  (1000000 / SERVO_FREQ_HZ)
#define MOTOR_PERIOD_TICKS 50  // 1 MHz / 20 kHz

static mcpwm_timer_handle_t s_motor_timer = NULL;
static mcpwm_oper_handle_t  s_motor_oper  = NULL;
static mcpwm_cmpr_handle_t  s_motor_cmpr  = NULL;
static mcpwm_gen_handle_t   s_motor_gen   = NULL;

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
    // MCPWM: motor ESC 20 kHz (API handle-based IDF v6.0+)
    mcpwm_timer_config_t timer_cfg = {
        .group_id      = 0,
        .clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000,
        .count_mode    = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks  = MOTOR_PERIOD_TICKS,
    };
    mcpwm_new_timer(&timer_cfg, &s_motor_timer);

    mcpwm_operator_config_t oper_cfg = { .group_id = 0 };
    mcpwm_new_operator(&oper_cfg, &s_motor_oper);
    mcpwm_operator_connect_timer(s_motor_oper, s_motor_timer);

    mcpwm_comparator_config_t cmpr_cfg = {};
    cmpr_cfg.flags.update_cmp_on_tez = true;
    mcpwm_new_comparator(s_motor_oper, &cmpr_cfg, &s_motor_cmpr);
    mcpwm_comparator_set_compare_value(s_motor_cmpr, 0);

    mcpwm_generator_config_t gen_cfg = { .gen_gpio_num = enable1Pin };
    mcpwm_new_generator(s_motor_oper, &gen_cfg, &s_motor_gen);
    mcpwm_generator_set_action_on_timer_event(s_motor_gen,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                     MCPWM_TIMER_EVENT_EMPTY,
                                     MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(s_motor_gen,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                       s_motor_cmpr,
                                       MCPWM_GEN_ACTION_LOW));
    mcpwm_timer_enable(s_motor_timer);
    mcpwm_timer_start_stop(s_motor_timer, MCPWM_TIMER_START_NO_STOP);

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
        ESP_LOGD("DC", "Motor speed: %d, Duty cycle: %.2f%%", speed, dutyCycle * 100.0 / 1024.0);
        mcpwm_comparator_set_compare_value(s_motor_cmpr,
            (uint32_t)(dutyCycle * MOTOR_PERIOD_TICKS / 1024.0f));
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
