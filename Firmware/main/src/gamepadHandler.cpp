#include "gamepadHandler.h"
#include "actuators.h"
#include "state.h"
#include "ProgramManager.h"
#include <Preferences.h>

extern Preferences preferences;

// Global pointers, initialized in setup
static VehicleState* g_state = nullptr;
static ProgramManager* g_programManager = nullptr;

// State for recording
static bool isRecording = false;
static int lastMotor = 0;
static int lastSteer = 0;

// Helper struct for button state
struct ButtonState {
    bool r1LastState = false;
    bool r1Pressed = false;
    bool l1LastState = false;
    bool l1Pressed = false;
    bool yLastState = false;
    bool yPressed = false;
    bool xLastState = false;
    bool xPressed = false;
    bool aLastState = false;
    bool aPressed = false;
    bool bLastState = false;
    bool bPressed = false;
};

static ButtonState btnState;

/**
 * @brief Inicializa el gestor de gamepads Bluepad32.
 */
void setupGamepad(VehicleState* state, ProgramManager* programManager) {
    g_state = state;
    g_programManager = programManager;
    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    // Habilita o deshabilita el descubrimiento de nuevos mandos según la configuración.
    (g_state->enableScan == 1) ? BP32.enableNewBluetoothConnections(true) : BP32.enableNewBluetoothConnections(false);
}

/**
 * @brief Callback que se ejecuta cuando un gamepad se conecta.
 */
void onConnectedGamepad(GamepadPtr gp) {
    if (!g_state) return;
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (g_state->myGamepads[i] == nullptr) {
            Console.printf("CALLBACK: Gamepad conectado, índice=%d\n", i);
            GamepadProperties properties = gp->getProperties();
            Console.printf("Modelo: %s, VID=0x%04x, PID=0x%04x\n", gp->getModelName(), properties.vendor_id,
                           properties.product_id);
            g_state->myGamepads[i] = gp;
            foundEmptySlot = true;
            // Deshabilita la conexión de nuevos mandos una vez que uno se ha conectado.
            BP32.enableNewBluetoothConnections(false);
            break;
        }
    }
    if (!foundEmptySlot) {
        Console.println("CALLBACK: Gamepad conectado, pero no hay slots libres.");
    }
}

/**
 * @brief Callback que se ejecuta cuando un gamepad se desconecta.
 */
void onDisconnectedGamepad(GamepadPtr gp) {
    if (!g_state) return;
    bool foundGamepad = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (g_state->myGamepads[i] == gp) {
            Console.printf("CALLBACK: Gamepad desconectado del índice=%d\n", i);
            g_state->myGamepads[i] = nullptr;
            foundGamepad = true;
            break;
        }
    }
    if (!foundGamepad) {
        Console.println("CALLBACK: Gamepad desconectado, pero no se encontró en la lista.");
    }
}

/**
 * @brief Controla el servo de dirección basado en la entrada del joystick.
 */
void servoController(GamepadPtr myGamepad, VehicleState* state) {
    int mov = myGamepad->axisX();
    // Aplica una pequeña "zona muerta" para evitar movimientos por ruido.
    if (abs(mov) < 20) mov = 0;
    setSteer(mov, state);
    lastSteer = mov;
}

/**
 * @brief Controla el motor basado en los gatillos de acelerador y freno.
 */
void motorController(GamepadPtr myGamepad, VehicleState* state) {
    int motorSpeed = 0;
    if (myGamepad->brake() > 20) {
        motorSpeed = -myGamepad->brake();
    } else if (myGamepad->throttle() > 20) {
        motorSpeed = myGamepad->throttle();
    }

    setMotor(abs(motorSpeed), motorSpeed >= 0, state);
    lastMotor = motorSpeed;
}

/**
 * @brief Controla las luces y otras acciones basado en los botones.
 */
void buttonController(GamepadPtr myGamepad, VehicleState* state, ProgramManager* programManager) {
    // Detección de flanco para el intermitente derecho (R1).
    if (myGamepad->r1() != btnState.r1LastState) {
        btnState.r1Pressed = true;
    } else {
        btnState.r1Pressed = false;
    }
    btnState.r1LastState = myGamepad->r1();
    if (btnState.r1Pressed && btnState.r1LastState) {
        state->giroDerecho = !state->giroDerecho;
    }

    // Detección de flanco para el intermitente izquierdo (L1).
    if (myGamepad->l1() != btnState.l1LastState) {
        btnState.l1Pressed = true;
    } else {
        btnState.l1Pressed = false;
    }
    btnState.l1LastState = myGamepad->l1();
    if (btnState.l1Pressed && btnState.l1LastState) {
        state->giroIzquierdo = !state->giroIzquierdo;
    }

    // Detección de flanco para las luces de emergencia (Y/Triángulo).
    if (myGamepad->y() != btnState.yLastState) {
        btnState.yPressed = true;
    } else {
        btnState.yPressed = false;
    }
    btnState.yLastState = myGamepad->y();
    if (btnState.yPressed && btnState.yLastState) {
        state->baliza = !state->baliza;
    }

    // Detección de flanco para ciclar los faros (X/Cuadrado).
    if (myGamepad->x() != btnState.xLastState) {
        btnState.xPressed = true;
    } else {
        btnState.xPressed = false;
    }
    btnState.xLastState = myGamepad->x();
    if (btnState.xPressed && btnState.xLastState) {
        state->luces++;
        if (state->luces > 3) state->luces = 0;
    }

    // Detección de flanco para el Botón A (Reproducir/Parar programa).
    if (myGamepad->a() != btnState.aLastState) {
        btnState.aPressed = true;
    } else {
        btnState.aPressed = false;
    }
    btnState.aLastState = myGamepad->a();
    if (btnState.aPressed && btnState.aLastState) {
        if (g_programManager->isRunning()) {
            g_programManager->stopProgram();
        } else if (!isRecording) {
            g_programManager->startProgram();
        }
    }

    // Detección de flanco para el Botón B (Iniciar/Parar grabación).
    if (myGamepad->b() != btnState.bLastState) {
        btnState.bPressed = true;
    } else {
        btnState.bPressed = false;
    }
    btnState.bLastState = myGamepad->b();
    if (btnState.bPressed && btnState.bLastState) {
        isRecording = !isRecording;
        if (isRecording) {
            g_programManager->startRecording();
        } else {
            g_programManager->stopRecording();
        }
    }
}

/**
 * @brief Procesa las entradas de los gamepads conectados.
 */
void handleGamepads(VehicleState* state, ProgramManager* programManager) {
    BP32.update();

    // Actualmente, la lógica solo soporta un gamepad a la vez.
    for (int i = 0; i < 1; i++) {
        GamepadPtr myGamepad = state->myGamepads[i];
        if (myGamepad && myGamepad->isConnected()) {
            servoController(myGamepad, state);
            motorController(myGamepad, state);
            buttonController(myGamepad, state, programManager);

            if (isRecording) {
                g_programManager->recordStep(lastMotor, lastSteer);
            }
        }
    }
}