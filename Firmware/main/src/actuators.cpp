/**
 * @file actuators.cpp
 * @brief Implementación del control de actuadores (motor y servo).
 */
#include "actuators.h"
#include <cstring>
#include "esp_log.h"
#include "esp_timer.h"
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

// Timestamp del último comando de calibración del ESC. El bloqueo de calibración
// se auto-libera tras este tiempo para que una calibración olvidada (sin "Finalizar")
// no deje el motor sin responder indefinidamente.
static int64_t s_esc_cal_last_us = 0;
#define ESC_CAL_TIMEOUT_US (60LL * 1000000LL)  // 60 s

// Secuencia automática de reversa para el ESC. Los ESC de auto interpretan el
// primer pulso de reversa como FRENO; sólo después de pasar por neutral el
// siguiente pulso de reversa es reversa real. updateEsc reproduce esa secuencia
// (FRENO→NEUTRAL→REVERSA) para que "tirar atrás" termine en reversa sin que el
// usuario tenga que soltar y volver a apretar.
enum EscPhase { ESC_PHASE_NEUTRAL, ESC_PHASE_FORWARD, ESC_PHASE_BRAKE, ESC_PHASE_REARM, ESC_PHASE_REVERSE };
static EscPhase s_esc_phase = ESC_PHASE_NEUTRAL;
static int64_t  s_esc_phase_us = 0;
// El ESC queda "cebado" para reversa tras reversar: mientras no se aplique avance,
// volver a pedir reversa (reversa→neutral→reversa) entra directo SIN repetir el baile
// freno→neutral. Sólo el avance resetea el cebado.
static bool s_esc_rev_primed = false;
// Tiempos de freno/rearme configurables (state->escBrakeMs / escRearmMs, en ms).

static void servo_write_angle(int degrees) {
    uint32_t pulse_us = SERVO_MIN_US +
        (uint32_t)((uint32_t)degrees * (SERVO_MAX_US - SERVO_MIN_US) / SERVO_MAX_ANGLE);
    uint32_t duty = (uint32_t)((uint64_t)pulse_us * SERVO_RES_MAX / SERVO_PERIOD_US);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, STEER_SERVO_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, STEER_SERVO_CHANNEL);
}

// Emite un pulso (en microsegundos) por el canal LEDC del ESC. El ESC brushless
// usa el mismo timer LEDC de 50 Hz / 14-bit que los servos.
static void esc_write_us(uint32_t pulse_us) {
    uint32_t duty = (uint32_t)((uint64_t)pulse_us * SERVO_RES_MAX / SERVO_PERIOD_US);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, ESC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, ESC_CHANNEL);
}

/**
 * @brief Configura e inicializa los pines y periféricos para los actuadores.
 */
void setupActuators(VehicleState* state) {
    // Timer LEDC compartido por todos los servos (y por el ESC): 50 Hz, 14-bit.
    ledc_timer_config_t servo_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = SERVO_RES_BITS,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = SERVO_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&servo_timer);

    // Servo de dirección vía LEDC directo (siempre).
    ledc_channel_config_t servo_ch = {
        .gpio_num   = (gpio_num_t)state->pinSteerServo,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = STEER_SERVO_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&servo_ch);

    if (state->motorType == 1) {
        // Motor brushless con ESC: señal PWM tipo servo (1000-2000 µs) por pinMotorEn.
        ledc_channel_config_t esc_ch = {
            .gpio_num   = (gpio_num_t)state->pinMotorEn,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel    = ESC_CHANNEL,
            .intr_type  = LEDC_INTR_DISABLE,
            .timer_sel  = LEDC_TIMER_0,
            .duty       = 0,
            .hpoint     = 0,
        };
        ledc_channel_config(&esc_ch);
        esc_write_us(state->escCenterUs);  // arma el ESC en neutral al arrancar
        ESP_LOGI("ESC", "Modo ESC brushless en GPIO %d (neutral=%u us, min=%u, max=%u)",
                 state->pinMotorEn, (unsigned)state->escCenterUs,
                 (unsigned)state->escMinUs, (unsigned)state->escMaxUs);
    } else {
        // Motor DC vía L298N: MCPWM a 20 kHz (API handle-based IDF v6.0+).
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

        mcpwm_generator_config_t gen_cfg = { .gen_gpio_num = state->pinMotorEn };
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
        gpio_set_direction((gpio_num_t)state->pinMotorDir1, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)state->pinMotorDir2, GPIO_MODE_OUTPUT);
    }
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

    if (state->motorType == 1) {
        // Motor brushless con ESC: sólo guarda el target; la salida física la
        // escribe updateEsc() cada tick (maneja la secuencia de reversa y el freno).
        int s = abs(speed);
        if (s > 1023) s = 1023;
        state->escTargetSpeed = s;
        state->escTargetForward = forward;
        // Un comando real de acelerador libera el freno mantenido.
        if (s > 0) state->brakeActive = false;
        return;
    }

    // Inversión de sentido opcional (DC).
    if (state->motorInvert) forward = !forward;

    float dutyCycle = 0;
    // Calcula el rango dinámico de velocidad.
    unsigned int motorMult = state->motorMaxSpeed - state->motorMinSpeed;

    if (speed != 0) {
        // Establece la dirección del motor.
        if (forward) {
            gpio_set_level((gpio_num_t)state->pinMotorDir1, 0);
            gpio_set_level((gpio_num_t)state->pinMotorDir2, 1);
        } else {
            gpio_set_level((gpio_num_t)state->pinMotorDir1, 1);
            gpio_set_level((gpio_num_t)state->pinMotorDir2, 0);
        }
        // Mapea la velocidad (0-1023) al rango de PWM configurado.
        dutyCycle = ((abs(speed) * motorMult) / 1024.0) + state->motorMinSpeed;
        ESP_LOGD("DC", "Motor speed: %d, Duty cycle: %.2f%%", speed, dutyCycle * 100.0 / 1024.0);
        mcpwm_comparator_set_compare_value(s_motor_cmpr,
            (uint32_t)(dutyCycle * MOTOR_PERIOD_TICKS / 1024.0f));
    } else {
        // Detiene el motor.
        gpio_set_level((gpio_num_t)state->pinMotorDir1, 0);
        gpio_set_level((gpio_num_t)state->pinMotorDir2, 0);
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
    if (state->motorType == 1) {
        // ESC: punto muerto = pulso neutral. Resetea target, freno y secuencia.
        state->escTargetSpeed = 0;
        state->brakeActive = false;
        s_esc_phase = ESC_PHASE_NEUTRAL;
        if (!state->escCalibrating) esc_write_us(state->escCenterUs);
    } else {
        gpio_set_level((gpio_num_t)state->pinMotorDir1, 0);
        gpio_set_level((gpio_num_t)state->pinMotorDir2, 0);
    }
    servo_write_angle(state->servoCenterDeg);
    // En modo hold la cámara mantiene su posición cuando el vehículo se detiene.
    if (!state->camHoldMode) centerCamServos(state);
}

void escCalibrateOutput(VehicleState* state, const char* phase) {
    if (state->motorType != 1) return;
    s_esc_cal_last_us = esp_timer_get_time();
    if (strcmp(phase, "high") == 0) {
        state->escCalibrating = true;
        esc_write_us(state->escMaxUs);
    } else if (strcmp(phase, "neutral") == 0) {
        state->escCalibrating = true;
        esc_write_us(state->escCenterUs);
    } else if (strcmp(phase, "low") == 0) {
        state->escCalibrating = true;
        esc_write_us(state->escMinUs);
    } else if (strcmp(phase, "end") == 0) {
        esc_write_us(state->escCenterUs);
        state->escCalibrating = false;
    }
    ESP_LOGI("ESC", "Calibración fase '%s' (calibrating=%d)", phase, (int)state->escCalibrating);
}

// Escala la magnitud de throttle (0-1023) al rango configurado [escMinSpeed, escMaxSpeed].
// Escala la magnitud (0-1023) al rango [lo, hi].
static uint32_t esc_scale_speed(int s, unsigned int lo, unsigned int hi) {
    if (s <= 0) return 0;
    if (hi <= lo) return (lo > 1023) ? 1023 : lo;
    uint32_t scaled = (uint32_t)((uint64_t)s * (hi - lo) / 1023) + lo;
    return scaled > 1023 ? 1023 : scaled;
}

// Pulso de reversa (µs) para una magnitud ya escalada.
static inline uint32_t esc_rev_pulse(VehicleState* state, uint32_t scaled) {
    return state->escCenterUs - (uint32_t)((uint64_t)scaled * (state->escCenterUs - state->escMinUs) / 1023);
}

// Log diagnóstico: imprime sólo cuando la velocidad/pulso cambian, para no inundar
// el monitor a 15 ms. raw = velocidad pedida (negativa = reversa).
static void esc_log_change(int raw, uint32_t scaled, uint32_t pulse) {
    static int s_last_raw = INT32_MIN;
    if (raw == s_last_raw) return;
    s_last_raw = raw;
    ESP_LOGI("ESC", "target=%d -> scaled=%u -> %u us", raw, (unsigned)scaled, (unsigned)pulse);
}

void updateEsc(VehicleState* state) {
    if (state->motorType != 1) return;

    if (state->escCalibrating) {
        // La calibración tiene la salida; se auto-libera por timeout si se olvidó "Finalizar".
        if (esp_timer_get_time() - s_esc_cal_last_us > ESC_CAL_TIMEOUT_US) {
            state->escCalibrating = false;
            ESP_LOGW("ESC", "Calibración auto-liberada por timeout");
        } else {
            return;
        }
    }

    // Freno mantenido: el pulso de reversa funciona como freno (primer toque = freno).
    if (state->brakeActive) {
        esc_write_us(state->escMinUs);
        s_esc_phase = ESC_PHASE_NEUTRAL;  // al soltar el freno NO salta a reversa
        return;
    }

    int s = state->escTargetSpeed;

    if (s == 0) {
        esc_write_us(state->escCenterUs);
        s_esc_phase = ESC_PHASE_NEUTRAL;
        return;
    }

    // Inversión de sentido opcional (configurable). El sentido físico también puede
    // corregirse intercambiando 2 de los 3 cables del brushless o el Motor Rotation del ESC.
    bool fwd = state->escTargetForward;
    if (state->motorInvert) fwd = !fwd;

    if (fwd) {
        uint32_t scaled = esc_scale_speed(s, state->escMinSpeed, state->escMaxSpeed);
        uint32_t pulse = state->escCenterUs +
            (uint32_t)((uint64_t)scaled * (state->escMaxUs - state->escCenterUs) / 1023);
        esc_log_change(s, scaled, pulse);
        esc_write_us(pulse);
        s_esc_phase = ESC_PHASE_FORWARD;
        s_esc_rev_primed = false;  // el avance resetea el cebado de reversa del ESC
        return;
    }

    // Reversa: secuencia automática FRENO → NEUTRAL → REVERSA, limitada por escMaxSpeedRev.
    int64_t now = esp_timer_get_time();
    int64_t brakeUs = (int64_t)state->escBrakeMs * 1000LL;
    int64_t rearmUs = (int64_t)state->escRearmMs * 1000LL;
    uint32_t scaledRev = esc_scale_speed(s, state->escMinSpeed, state->escMaxSpeedRev);
    uint32_t revPulse = esc_rev_pulse(state, scaledRev);
    // El freno/tirón inicial también se capa al límite de reversa para que la reversa
    // nunca supere escMaxSpeedRev (aun si el ESC ya está cebado para reversa).
    uint32_t brakePulse = esc_rev_pulse(state, esc_scale_speed(1023, state->escMinSpeed, state->escMaxSpeedRev));
    switch (s_esc_phase) {
        case ESC_PHASE_NEUTRAL:
        case ESC_PHASE_FORWARD:
            if (s_esc_rev_primed) {
                // Venimos de reversa (sin avance en el medio): el ESC sigue cebado,
                // reversa directa sin repetir freno→neutral.
                s_esc_phase = ESC_PHASE_REVERSE;
                esc_log_change(-s, scaledRev, revPulse);
                esc_write_us(revPulse);
            } else {
                s_esc_phase = ESC_PHASE_BRAKE;
                s_esc_phase_us = now;
                esc_write_us(brakePulse);  // freno (capado al límite de reversa)
            }
            break;
        case ESC_PHASE_BRAKE:
            if (now - s_esc_phase_us >= brakeUs) {
                s_esc_phase = ESC_PHASE_REARM;
                s_esc_phase_us = now;
                esc_write_us(state->escCenterUs);  // rearme: neutral
            } else {
                esc_write_us(brakePulse);
            }
            break;
        case ESC_PHASE_REARM:
            if (now - s_esc_phase_us >= rearmUs) {
                s_esc_phase = ESC_PHASE_REVERSE;
                esc_write_us(revPulse);
            } else {
                esc_write_us(state->escCenterUs);
            }
            break;
        case ESC_PHASE_REVERSE:
            esc_log_change(-s, scaledRev, revPulse);
            esc_write_us(revPulse);  // reversa real, escalada por velocidad
            s_esc_rev_primed = true;  // ESC cebado: próxima reversa (tras neutral) será directa
            break;
    }
}

void brakeMotor(VehicleState* state, bool on) {
    state->brakeActive = on;
    if (state->motorType == 1) {
        // En ESC lo aplica updateEsc (lee brakeActive); aplicamos ya para baja latencia.
        if (on && !state->escCalibrating) esc_write_us(state->escMinUs);
        if (on) s_esc_phase = ESC_PHASE_NEUTRAL;
    } else {
        // Freno activo del L298N: ambos pines de dirección en alto con EN a tope.
        if (on) {
            gpio_set_level((gpio_num_t)state->pinMotorDir1, 1);
            gpio_set_level((gpio_num_t)state->pinMotorDir2, 1);
            mcpwm_comparator_set_compare_value(s_motor_cmpr, MOTOR_PERIOD_TICKS);
        } else {
            gpio_set_level((gpio_num_t)state->pinMotorDir1, 0);
            gpio_set_level((gpio_num_t)state->pinMotorDir2, 0);
            mcpwm_comparator_set_compare_value(s_motor_cmpr, 0);
        }
    }
}

// ── Camera servo helpers ───────────────────────────────────────────────────

static void cam_servo_write(ledc_channel_t ch, unsigned int degrees,
                            unsigned int min_us, unsigned int max_us) {
    uint32_t pulse_us = min_us + (uint32_t)((uint32_t)degrees * (max_us - min_us) / SERVO_MAX_ANGLE);
    uint32_t duty = (uint32_t)((uint64_t)pulse_us * SERVO_RES_MAX / SERVO_PERIOD_US);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, ch);
}

void setupCamServos(VehicleState* state) {
    if (!state->camServoEnabled) return;

    ledc_channel_config_t pan_ch = {
        .gpio_num   = (gpio_num_t)state->pinPanServo,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PAN_SERVO_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&pan_ch);

    ledc_channel_config_t tilt_ch = {
        .gpio_num   = (gpio_num_t)state->pinTiltServo,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = TILT_SERVO_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&tilt_ch);

    centerCamServos(state);
}

// Timestamp of last updateCamServos call; 0 = not yet called (dt treated as 0).
static int64_t s_cam_last_us = 0;

// setCamPan / setCamTilt only store the angle target.
// The actual LEDC write happens in updateCamServos(), called every 15 ms from
// the main loop. This decouples the physical servo update rate from the WS/gamepad
// command rate, eliminating the advance/pause stepping visible at 100 ms WS intervals.

void setCamPan(int angle, VehicleState* state) {
    if (!state->camServoEnabled) return;
    if (angle >  512) angle =  512;
    if (angle < -512) angle = -512;
    if (state->panInvert) angle = -angle;
    state->lastPanAngle = angle;
}

void setCamTilt(int angle, VehicleState* state) {
    if (!state->camServoEnabled) return;
    if (angle >  512) angle =  512;
    if (angle < -512) angle = -512;
    if (state->tiltInvert) angle = -angle;
    state->lastTiltAngle = angle;
}

void updateCamServos(VehicleState* state) {
    if (!state->camServoEnabled) return;

    int64_t now = esp_timer_get_time();
    float dt = (s_cam_last_us == 0) ? 0.0f : (float)(now - s_cam_last_us) / 1000000.0f;
    s_cam_last_us = now;
    if (dt > 0.1f) dt = 0.1f;

    int panDeg, tiltDeg;

    if (state->camHoldMode) {
        state->panPosDeg += (float)state->camHoldSpeed * ((float)state->lastPanAngle / 512.0f) * dt;
        float panMin = (float)state->panCenterDeg - (float)state->panLimitLDeg;
        float panMax = (float)state->panCenterDeg + (float)state->panLimitRDeg;
        if (state->panPosDeg < panMin) state->panPosDeg = panMin;
        if (state->panPosDeg > panMax) state->panPosDeg = panMax;
        panDeg = (int)(state->panPosDeg + 0.5f);

        // Negative angle = tilt up = increasing degrees (sign inversion matches absolute mode)
        state->tiltPosDeg += (float)state->camHoldSpeed * (-(float)state->lastTiltAngle / 512.0f) * dt;
        float tiltMin = (float)state->tiltCenterDeg - (float)state->tiltLimitDownDeg;
        float tiltMax = (float)state->tiltCenterDeg + (float)state->tiltLimitUpDeg;
        if (state->tiltPosDeg < tiltMin) state->tiltPosDeg = tiltMin;
        if (state->tiltPosDeg > tiltMax) state->tiltPosDeg = tiltMax;
        tiltDeg = (int)(state->tiltPosDeg + 0.5f);
    } else {
        float panPorc = (float)state->lastPanAngle / 512.0f;
        panDeg = (int)state->panCenterDeg;
        if (state->lastPanAngle < 0)
            panDeg = (int)state->panCenterDeg + (int)((float)state->panLimitLDeg * panPorc);
        else if (state->lastPanAngle > 0)
            panDeg = (int)state->panCenterDeg + (int)((float)state->panLimitRDeg * panPorc);
        state->panPosDeg = (float)panDeg;  // keep in sync for seamless switch to hold mode

        float tiltPorc = (float)state->lastTiltAngle / 512.0f;
        tiltDeg = (int)state->tiltCenterDeg;
        if (state->lastTiltAngle < 0)
            tiltDeg = (int)state->tiltCenterDeg - (int)((float)state->tiltLimitUpDeg * tiltPorc);
        else if (state->lastTiltAngle > 0)
            tiltDeg = (int)state->tiltCenterDeg - (int)((float)state->tiltLimitDownDeg * tiltPorc);
        state->tiltPosDeg = (float)tiltDeg;  // keep in sync for seamless switch to hold mode
    }

    if (panDeg < 0) panDeg = 0;
    if (panDeg > SERVO_MAX_ANGLE) panDeg = SERVO_MAX_ANGLE;
    if (tiltDeg < 0) tiltDeg = 0;
    if (tiltDeg > SERVO_MAX_ANGLE) tiltDeg = SERVO_MAX_ANGLE;

    cam_servo_write(PAN_SERVO_CHANNEL,  (unsigned int)panDeg,  state->panMinUs,  state->panMaxUs);
    cam_servo_write(TILT_SERVO_CHANNEL, (unsigned int)tiltDeg, state->tiltMinUs, state->tiltMaxUs);
}

void centerCamServos(VehicleState* state) {
    if (!state->camServoEnabled) return;
    cam_servo_write(PAN_SERVO_CHANNEL,  state->panCenterDeg,  state->panMinUs,  state->panMaxUs);
    cam_servo_write(TILT_SERVO_CHANNEL, state->tiltCenterDeg, state->tiltMinUs, state->tiltMaxUs);
    state->lastPanAngle  = 0;
    state->lastTiltAngle = 0;
    state->panPosDeg  = (float)state->panCenterDeg;
    state->tiltPosDeg = (float)state->tiltCenterDeg;
    s_cam_last_us = 0;  // reset dt so next updateCamServos starts fresh from center
}
