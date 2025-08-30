/**
 * @file state.h
 * @brief Definiciones de las estructuras de estado y tipos de datos del proyecto.
 * 
 * Este archivo centraliza la definición de la estructura `VehicleState`, que contiene
 * toda la información sobre el estado actual del vehículo, así como enumeraciones
 * y configuraciones relacionadas.
 */
#pragma once

#include <Bluepad32.h>
#include <vector>

// Declaración anticipada para evitar dependencias circulares.
struct VehicleState;

/**
 * @enum LedFunction
 * @brief Enumeración de las posibles funciones lógicas de un grupo de LEDs.
 */
enum LedFunction {
    LUZ_POSICION_DELANTERA,
    LUZ_POSICION_TRASERA,
    LUZ_FRENO,
    LUZ_REVERSA,
    LUZ_GIRO_IZQUIERDA,
    LUZ_GIRO_DERECHA,
    ILUMINACION_INTERIOR,
    NEON_INFERIOR,
    // ---
    LED_FUNCTION_COUNT
};

/**
 * @struct LedGroup
 * @brief Estructura para almacenar la configuración de un grupo funcional de LEDs.
 */
typedef struct {
    LedFunction funcion;    //< La función lógica de este grupo de LEDs.
    char leds[64];          //< String con los índices de LEDs (ej: "1-5, 8, 10-12").
    uint8_t colorR;         //< Componente rojo del color (0-255).
    uint8_t colorG;         //< Componente verde del color (0-255).
    uint8_t colorB;         //< Componente azul del color (0-255).
    uint8_t brillo;         //< Brillo del grupo (0-100).
} LedGroup;

/**
 * @struct VehicleState
 * @brief Estructura principal que contiene todo el estado del vehículo.
 */
struct VehicleState {
    // --- Estado del Gamepad ---
    GamepadPtr myGamepads[BP32_MAX_GAMEPADS];

    // --- Estado de los LEDs ---
    bool giroDerecho;           //< Intermitente derecho activado.
    bool giroIzquierdo;         //< Intermitente izquierdo activado.
    bool baliza;                //< Luces de emergencia activadas.
    int luces;                  //< Nivel de luces: 0=off, 1=posición, 2=bajas, 3=altas.
    bool brakesLedOn;           //< Luces de freno activadas.
    bool reverseLedOn;          //< Luces de reversa activadas.
    uint32_t ledCount;          //< Número total de LEDs en la tira.
    std::vector<LedGroup> ledGroups; //< Vector dinámico con la configuración de los grupos de LEDs.

    // --- Estado de Motor y Servo ---
    unsigned int servoCenterDeg;    //< Grados de centrado del servo.
    unsigned int servoLimitLDeg;    //< Límite de giro a la izquierda (grados desde el centro).
    unsigned int servoLimitRDeg;    //< Límite de giro a la derecha (grados desde el centro).
    unsigned int motorMinSpeed;     //< Velocidad mínima del motor (valor PWM).
    unsigned int motorMaxSpeed;     //< Velocidad máxima del motor (valor PWM).

    // --- Estado de la API ---
    bool apiActEnabled;             //< Indica si una acción de la API está en curso.
    unsigned long apiActMSStart;    //< Timestamp de inicio de la acción de la API.
    unsigned int apiActMS;          //< Duración de la acción de la API.
    unsigned int apiActMSTimeout;   //< Timeout por defecto para las acciones de la API.

    // --- Estado del WiFi ---
    char wifiName[32];      //< SSID de la red WiFi.
    char wifiPass[64];      //< Contraseña de la red WiFi.
    char wifiMode[4];       //< Modo WiFi ("AP" o "STA").
    char espIP[16];         //< Dirección IP actual del ESP32.
    char cameraIP[16];      //< Dirección IP de la cámara (si aplica).

    // --- Escaneo Bluetooth ---
    unsigned int enableScan;    //< Habilitar/deshabilitar el escaneo de nuevos mandos.
};
