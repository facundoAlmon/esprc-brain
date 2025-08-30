/**
 * @file webServerHandler.h
 * @brief Declaraciones para el servidor web y la API REST.
 * 
 * Define las funciones para iniciar y detener el servidor web que sirve la
 * interfaz de usuario y gestiona las peticiones de la API.
 */
#pragma once

#include "state.h"

/**
 * @brief Inicia el servidor web y registra todos los endpoints.
 * 
 * Configura y arranca el servidor HTTP, incluyendo los endpoints para la web,
 * la API REST y el WebSocket.
 * @param state Puntero al estado global del vehículo.
 */
void startServer(VehicleState* state);

/**
 * @brief Detiene el servidor web.
 */
void stopServer();




