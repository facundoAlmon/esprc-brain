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
void setupActuators();

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
 * @brief Detiene todos los actuadores.
 * 
 * Pone el motor en punto muerto y centra el servo de dirección.
 * @param state Puntero al estado global del vehículo.
 */
void stopMotors(VehicleState* state);
