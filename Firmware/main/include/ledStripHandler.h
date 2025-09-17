/**
 * @file ledStripHandler.h
 * @brief Declaraciones para el control de la tira de LEDs.
 * 
 * Define las funciones para inicializar y manejar la tira de LEDs direccionables (WS2812).
 */
#pragma once

#include "state.h"
#include "led_strip.h"
#include "ProgramManager.h"

/**
 * @brief Handle global para la tira de LEDs.
 */
extern led_strip_handle_t led_strip;

/**
 * @brief Configura e inicializa la tira de LEDs.
 * 
 * @param state Puntero al estado global del vehículo.
 */
void setupLedStrip(VehicleState* state);

/**
 * @brief Actualiza el estado de la tira de LEDs.
 * 
 * Esta función debe ser llamada en el bucle principal para reflejar el estado
 * actual del vehículo (intermitentes, luces, frenos) en los LEDs.
 * @param state Puntero al estado global del vehículo.
 * @param programManager Puntero al gestor de programas.
 */
void handleLedStrip(VehicleState* state, ProgramManager* programManager);
