/**
 * @file actuators.h
 * @brief Declaraciones de funciones para controlar los actuadores del vehículo.
 * 
 * Este archivo contiene las prototipos de las funciones para inicializar y controlar
 * el motor de tracción y el servo de dirección.
 */
#pragma once

#include "state.h"

/**
 * @brief Configura e inicializa los pines y periféricos para los actuadores.
 * 
 * Inicializa el hardware MCPWM para el control de velocidad del motor y el
 * servo para la dirección.
 */
void setupActuators(VehicleState* state);

/**
 * @brief Establece la velocidad y dirección del motor.
 * 
 * @param speed La velocidad del motor (0-1023).
 * @param forward `true` para avanzar, `false` para retroceder.
 * @param state Puntero al estado global del vehículo.
 */
void setMotor(int speed, bool forward, VehicleState* state);

/**
 * @brief Establece el ángulo de la dirección.
 *
 * @param angle El ángulo de giro (-512 a 512).
 * @param state Puntero al estado global del vehículo.
 */
void setSteer(int angle, VehicleState* state);

/**
 * @brief Emite un pulso de calibración fijo en la señal del ESC.
 *
 * El firmware no puede apagar/encender el ESC, así que la calibración consiste en
 * mantener un pulso de tope mientras el usuario sigue la secuencia física del ESC.
 * No hace nada si el tipo de motor no es brushless (motorType != 1).
 *
 * @param state Puntero al estado global del vehículo.
 * @param phase "high" (avance máx), "neutral", "low" (reversa máx) o "end" (vuelve a neutral y libera).
 */
void escCalibrateOutput(VehicleState* state, const char* phase);

/**
 * @brief Actualiza la salida física del ESC brushless. Debe llamarse cada tick del
 *        loop principal (~15 ms). Lee el target fijado por setMotor y aplica la
 *        secuencia automática freno→neutral→reversa, el escalado de velocidad y el
 *        freno mantenido. No hace nada si motorType != 1.
 */
void updateEsc(VehicleState* state);

/**
 * @brief Activa/desactiva el freno mantenido. En modo ESC lo aplica updateEsc;
 *        en modo DC frena activamente el L298N (ambos pines de dirección en alto).
 */
void brakeMotor(VehicleState* state, bool on);

/**
 * @brief Detiene todos los actuadores.
 *
 * Pone el motor en punto muerto y centra el servo de dirección.
 * @param state Puntero al estado global del vehículo.
 */
void stopMotors(VehicleState* state);

/**
 * @brief Inicializa los servos de cámara (pan y tilt) si están habilitados.
 */
void setupCamServos(VehicleState* state);

/**
 * @brief Mueve el servo de pan al ángulo dado.
 * @param angle Ángulo de -512 a 512 (negativo = izquierda, positivo = derecha).
 */
void setCamPan(int angle, VehicleState* state);

/**
 * @brief Mueve el servo de tilt al ángulo dado.
 * @param angle Ángulo de -512 a 512 (negativo = arriba, positivo = abajo).
 */
void setCamTilt(int angle, VehicleState* state);

/**
 * @brief Centra ambos servos de cámara.
 */
void centerCamServos(VehicleState* state);

/**
 * @brief Actualiza la posición física de los servos de cámara. Debe llamarse
 *        cada tick del loop principal (~15 ms). Aplica integración en modo hold
 *        o tracking directo en modo absoluto usando el último ángulo recibido.
 */
void updateCamServos(VehicleState* state);
