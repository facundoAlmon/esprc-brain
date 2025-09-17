#pragma once

#include <Bluepad32.h>
#include "state.h"
#include "ProgramManager.h"

/**
 * @file gamepadHandler.h
 * @brief Declaraciones para la gestión de mandos de videojuegos (Gamepads) con Bluepad32.
 * 
 * Define las funciones para inicializar el sistema de gamepad, manejar las conexiones/desconexiones
 * y procesar las entradas de los mandos en el bucle principal.
 */

/**
 * @brief Inicializa el gestor de gamepads Bluepad32.
 * 
 * @param state Puntero al estado global del vehículo.
 * @param programManager Puntero al gestor de programas.
 */
void setupGamepad(VehicleState* state, ProgramManager* programManager);

/**
 * @brief Callback que se ejecuta cuando un gamepad se conecta.
 * 
 * @param gp Puntero al objeto Gamepad que se ha conectado.
 */
void onConnectedGamepad(GamepadPtr gp);

/**
 * @brief Callback que se ejecuta cuando un gamepad se desconecta.
 * 
 * @param gp Puntero al objeto Gamepad que se ha desconectado.
 */
void onDisconnectedGamepad(GamepadPtr gp);

/**
 * @brief Procesa las entradas de los gamepads conectados.
 * 
 * Esta función debe ser llamada en cada iteración del bucle principal para leer
 * los estados de los joysticks y botones y actualizar el estado del vehículo.
 * @param state Puntero al estado global del vehículo.
 * @param programManager Puntero al gestor de programas.
 */
void handleGamepads(VehicleState* state, ProgramManager* programManager);


