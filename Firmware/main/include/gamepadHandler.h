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
 * @brief Procesa las entradas de movimiento de los gamepads (joysticks/gatillos).
 * 
 * Esta función debe ser llamada en cada iteración del bucle principal para leer
 * los estados de los joysticks y gatillos y actualizar el estado del vehículo.
 * Se debe llamar solo cuando no se esté ejecutando un programa.
 * @param state Puntero al estado global del vehículo.
 */
void handleGamepadMotion(VehicleState* state);

/**
 * @brief Procesa las entradas de los botones de los gamepads.
 * 
 * Esta función debe ser llamada en cada iteración del bucle principal para leer
 * el estado de los botones para acciones como luces, grabación, etc.
 * @param state Puntero al estado global del vehículo.
 * @param programManager Puntero al gestor de programas.
 */
void handleGamepadButtons(VehicleState* state, ProgramManager* programManager);


