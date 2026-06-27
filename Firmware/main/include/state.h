/**
 * @file state.h
 * @brief Definiciones de las estructuras de estado y tipos de datos del proyecto.
 * 
 * Este archivo centraliza la definición de la estructura `VehicleState`, que contiene
 * toda la información sobre el estado actual del vehículo, así como enumeraciones
 * y configuraciones relacionadas.
 */
#pragma once

#include "gamepad_types.h"
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
    REC_INDICATOR,          //< Indicador de grabación
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
    GamepadData myGamepads[MAX_GAMEPADS];

    // --- Estado de los LEDs ---
    bool giroDerecho;           //< Intermitente derecho activado.
    bool giroIzquierdo;         //< Intermitente izquierdo activado.
    bool baliza;                //< Luces de emergencia activadas.
    int luces;                  //< Nivel de luces: 0=off, 1=posición, 2=bajas, 3=altas.
    bool brakesLedOn;           //< Luces de freno activadas.
    bool reverseLedOn;          //< Luces de reversa activadas.
    bool autoTurnSignals;       //< Habilita el encendido/apagado automático de intermitentes.
    unsigned int autoTurnTol; //< Umbral de activación de intermitentes automáticos (%).
    uint32_t ledCount;          //< Número total de LEDs en la tira.
    std::vector<LedGroup> ledGroups; //< Vector dinámico con la configuración de los grupos de LEDs.

    // --- Estado de Motor y Servo ---
    unsigned int servoCenterDeg;    //< Grados de centrado del servo.
    unsigned int servoLimitLDeg;    //< Límite de giro a la izquierda (grados desde el centro).
    unsigned int servoLimitRDeg;    //< Límite de giro a la derecha (grados desde el centro).
    unsigned int motorMinSpeed;     //< Velocidad mínima del motor (valor PWM).
    unsigned int motorMaxSpeed;     //< Velocidad máxima del motor (valor PWM).
    unsigned int motorType;         //< Tipo de motor: 0 = DC (L298N), 1 = Brushless (ESC).
    unsigned int escMinUs;          //< Pulso de reversa máxima del ESC (microsegundos).
    unsigned int escCenterUs;       //< Pulso neutral del ESC (microsegundos).
    unsigned int escMaxUs;          //< Pulso de avance máximo del ESC (microsegundos).
    unsigned int escMinSpeed;       //< Throttle mínimo del ESC (0-1023) para vencer la zona muerta.
    unsigned int escMaxSpeed;       //< Throttle máximo del ESC (0-1023): limita la velocidad tope de avance.
    unsigned int escMaxSpeedRev;    //< Throttle máximo de reversa del ESC (0-1023), independiente del avance.
    unsigned int escBrakeMs;        //< Duración del freno inicial antes de rearmar reversa (ms).
    unsigned int escRearmMs;        //< Duración del neutral de rearme antes de aplicar reversa (ms).
    bool motorInvert;               //< Invierte el sentido de marcha (DC y ESC).
    bool escCalibrating;            //< Runtime-only: bloquea setMotor durante la calibración del ESC (no se persiste).
    int  escTargetSpeed;            //< Runtime-only: magnitud de throttle ESC deseada (0-1023); la escribe updateEsc.
    bool escTargetForward;          //< Runtime-only: dirección deseada del ESC (true=adelante).
    bool brakeActive;               //< Runtime-only: freno mantenido activo (botón de freno).
    unsigned int gpThrottleDZ;      //< Zona muerta del gatillo de acelerador/freno del gamepad BT.
    unsigned int gpSteerDZ;         //< Zona muerta del stick de dirección del gamepad BT.
    int lastSteerDirection;     //< Última dirección de giro: -1 izq, 0 centro, 1 der.
    int lastMotorSpeed;         //< Última velocidad del motor.
    bool lastMotorForward;      //< Última dirección del motor.

    // --- Servos de Cámara (Pan/Tilt) ---
    bool         camServoEnabled;       //< Habilita los servos de cámara.
    bool         camGamepadEnabled;     //< Habilita control de cámara con gamepad (stick derecho).
    bool         panInvert;             //< Invierte el eje pan.
    bool         tiltInvert;            //< Invierte el eje tilt.
    unsigned int camStickDZ;            //< Deadzone del stick de gamepad para cámara (0-100).
    unsigned int camStickSat;           //< Saturación del stick: a este valor el servo llega al límite.
    unsigned int panCenterDeg;          //< Ángulo central del servo pan.
    unsigned int panLimitLDeg;          //< Límite de giro a la izquierda del servo pan.
    unsigned int panLimitRDeg;          //< Límite de giro a la derecha del servo pan.
    unsigned int panMinUs;              //< Pulso mínimo del servo pan (microsegundos).
    unsigned int panMaxUs;              //< Pulso máximo del servo pan (microsegundos).
    unsigned int tiltCenterDeg;         //< Ángulo central del servo tilt.
    unsigned int tiltLimitUpDeg;        //< Límite de giro hacia arriba del servo tilt.
    unsigned int tiltLimitDownDeg;      //< Límite de giro hacia abajo del servo tilt.
    unsigned int tiltMinUs;             //< Pulso mínimo del servo tilt (microsegundos).
    unsigned int tiltMaxUs;             //< Pulso máximo del servo tilt (microsegundos).
    int          lastPanAngle;          //< Último ángulo de pan (-512..512).
    int          lastTiltAngle;         //< Último ángulo de tilt (-512..512).
    bool         camHoldMode;           //< Modo hold: el stick mueve incrementalmente y el servo mantiene posición.
    unsigned int camHoldSpeed;          //< Velocidad máxima en modo hold (grados/segundo).
    float        panPosDeg;             //< Posición actual del pan en modo hold (grados).
    float        tiltPosDeg;            //< Posición actual del tilt en modo hold (grados).

    // --- Estado de la API ---
    bool apiActEnabled;             //< Indica si una acción de la API está en curso.
    unsigned long apiCamActUntil;   //< millis() hasta el cual la webapp/API tiene prioridad sobre los servos de cámara.
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

    // --- Pines GPIO (configurables por NVS, defaults en pins.h) ---
    int pinLedStrip;
    int pinMotorEn;
    int pinMotorDir1;
    int pinMotorDir2;
    int pinSteerServo;
    int pinPanServo;
    int pinTiltServo;
};
