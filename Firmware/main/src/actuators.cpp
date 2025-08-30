/**
 * @file actuators.cpp
 * @brief Implementación del control de actuadores (motor y servo).
 */
#include "actuators.h"
#include "driver/mcpwm.h"
#include "iot_servo.h"
#include "soc/mcpwm_periph.h"
#include "pins.h"

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
    pinMode(motor1Pin1, OUTPUT);
    pinMode(motor1Pin2, OUTPUT);

    // Configura el servo de dirección.
    servo_config_t servo_cfg = {
        .max_angle = 180,
        .min_width_us = 500,
        .max_width_us = 2500,
        .freq = 50,
        .timer_number = LEDC_TIMER_0,
        .channels =
            {
                .servo_pin =
                    {
                        servoPin,
                    },
                .ch =
                    {
                        servoChannel,
                    },
            },
        .channel_number = 1,
    };
    iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
}

/**
 * @brief Establece la velocidad y dirección del motor.
 */
void setMotor(int speed, bool forward, VehicleState* state) {
    float dutyCycle = 0;
    // Calcula el rango dinámico de velocidad.
    unsigned int motorMult = state->motorMaxSpeed - state->motorMinSpeed;

    if (speed != 0) {
        // Establece la dirección del motor.
        if (forward) {
            digitalWrite(motor1Pin1, HIGH);
            digitalWrite(motor1Pin2, LOW);
        } else {
            digitalWrite(motor1Pin1, LOW);
            digitalWrite(motor1Pin2, HIGH);
        }
        // Mapea la velocidad (0-1023) al rango de PWM configurado.
        dutyCycle = ((speed * motorMult) / 1024.0) + state->motorMinSpeed;
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, dutyCycle * 100.0 / 255.0);
        mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    } else {
        // Detiene el motor.
        digitalWrite(motor1Pin1, LOW);
        digitalWrite(motor1Pin2, LOW);
    }
}

/**
 * @brief Establece el ángulo de la dirección.
 */
void setSteer(int angle, VehicleState* state) {
    float porc = (float)angle / 512.0; // Normaliza el ángulo a un rango de -1.0 a 1.0.
    int posDegrees = 0;

    if (angle == 0) {
        posDegrees = state->servoCenterDeg;
    } else if (porc < 0) { // Izquierda
        float movS = (state->servoLimitLDeg * porc);
        posDegrees = state->servoCenterDeg + movS;
    } else { // Derecha
        float movS = (state->servoLimitRDeg * porc);
        posDegrees = state->servoCenterDeg + movS;
    }
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, servoChannel, posDegrees);
}

/**
 * @brief Detiene todos los actuadores.
 */
void stopMotors(VehicleState* state) {
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, servoChannel, state->servoCenterDeg);
}
