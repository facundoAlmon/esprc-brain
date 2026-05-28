#include "gamepadHandler.h"
#include "actuators.h"
#include "state.h"
#include "ProgramManager.h"
#include "ledStripHandler.h"
#include "hid_gamepad.h"
#include "esp_log.h"

static const char* TAG = "Gamepad";

static VehicleState* g_state = nullptr;
static ProgramManager* g_programManager = nullptr;

static bool isRecording = false;
static int lastMotor = 0;
static int lastSteer = 0;

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

void setupGamepad(VehicleState* state, ProgramManager* programManager) {
    g_state = state;
    g_programManager = programManager;
    hid_gamepad_init(state);
    hid_gamepad_set_scanning(state->enableScan == 1);
}

static void servoController(const GamepadData* gp, VehicleState* state) {
    int mov = gp->axis_x;
    if (abs(mov) < 20) mov = 0;
    setSteer(mov, state);
    lastSteer = mov;
}

static void motorController(const GamepadData* gp, VehicleState* state) {
    int motorSpeed = 0;
    if (gp->brake > 20) {
        motorSpeed = -gp->brake;
    } else if (gp->throttle > 20) {
        motorSpeed = gp->throttle;
    }
    setMotor(abs(motorSpeed), motorSpeed >= 0, state);
    lastMotor = motorSpeed;
}

static void buttonController(const GamepadData* gp, VehicleState* state, ProgramManager* pm) {
    bool r1 = (gp->buttons & GAMEPAD_BTN_R1) != 0;
    if (r1 != btnState.r1LastState) btnState.r1Pressed = true;
    else btnState.r1Pressed = false;
    btnState.r1LastState = r1;
    if (btnState.r1Pressed && btnState.r1LastState) state->giroDerecho = !state->giroDerecho;

    bool l1 = (gp->buttons & GAMEPAD_BTN_L1) != 0;
    if (l1 != btnState.l1LastState) btnState.l1Pressed = true;
    else btnState.l1Pressed = false;
    btnState.l1LastState = l1;
    if (btnState.l1Pressed && btnState.l1LastState) state->giroIzquierdo = !state->giroIzquierdo;

    bool y = (gp->buttons & GAMEPAD_BTN_Y) != 0;
    if (y != btnState.yLastState) btnState.yPressed = true;
    else btnState.yPressed = false;
    btnState.yLastState = y;
    if (btnState.yPressed && btnState.yLastState) state->baliza = !state->baliza;

    bool x = (gp->buttons & GAMEPAD_BTN_X) != 0;
    if (x != btnState.xLastState) btnState.xPressed = true;
    else btnState.xPressed = false;
    btnState.xLastState = x;
    if (btnState.xPressed && btnState.xLastState) {
        state->luces++;
        if (state->luces > 3) state->luces = 0;
    }

    bool a = (gp->buttons & GAMEPAD_BTN_A) != 0;
    if (a != btnState.aLastState) btnState.aPressed = true;
    else btnState.aPressed = false;
    btnState.aLastState = a;
    if (btnState.aPressed && btnState.aLastState) {
        if (pm->isRunning()) pm->stopProgram();
        else if (!isRecording) pm->startProgram();
    }

    bool b = (gp->buttons & GAMEPAD_BTN_B) != 0;
    if (b != btnState.bLastState) btnState.bPressed = true;
    else btnState.bPressed = false;
    btnState.bLastState = b;
    if (btnState.bPressed && btnState.bLastState) {
        isRecording = !isRecording;
        if (isRecording) pm->startRecording();
        else pm->stopRecording();
    }
}

void handleGamepadButtons(VehicleState* state, ProgramManager* programManager) {
    // Data is pushed via bp32_platform callbacks — no polling call needed.
    const GamepadData* gp = &state->myGamepads[0];
    if (gp->connected) {
        buttonController(gp, state, programManager);
    }
}

void handleGamepadMotion(VehicleState* state) {
    const GamepadData* gp = &state->myGamepads[0];
    if (gp->connected) {
        servoController(gp, state);
        motorController(gp, state);

        if (isRecording) {
            g_programManager->recordStep(lastMotor, lastSteer);
        }
    }
}
