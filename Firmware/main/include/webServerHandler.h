/**
 * @file webServerHandler.h
 * @brief Declaraciones para el servidor web y la API REST.
 * 
 * Define las funciones para iniciar y detener el servidor web que sirve la
 * interfaz de usuario y gestiona las peticiones de la API.
 */
#pragma once

#include "state.h"

#include "ProgramManager.h"

/**
 * @brief Inicia el servidor web y registra todos los endpoints.
 * 
 * Configura y arranca el servidor HTTP, incluyendo los endpoints para la web,
 * la API REST y el WebSocket.
 * @param state Puntero al estado global del vehículo.
 * @param programManager Puntero al gestor de programas.
 */
void startServer(VehicleState* state, ProgramManager* programManager);

/**
 * @brief Detiene el servidor web.
 */
void stopServer();




