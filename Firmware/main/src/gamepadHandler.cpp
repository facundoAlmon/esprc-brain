#include "gamepadHandler.h"
#include "actuators.h"
#include "state.h"
#include "ProgramManager.h"
#include "ledStripHandler.h"
#include "hid_gamepad.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"

static inline uint32_t millis() { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

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
    bool thumbRLastState = false;
    bool thumbRPressed = false;
    bool selectLastState = false;
    bool selectPressed = false;
};

static void nvs_persist_bool(const char* key, bool v) {
    nvs_handle_t h;
    if (nvs_open("bl-car", NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u8(h, key, v ? 1 : 0);
    nvs_commit(h);
    nvs_close(h);
}

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

static int scale_cam_stick(int raw, int dz, int sat) {
    if (abs(raw) < dz) return 0;
    if (sat <= 0) sat = 1;
    int out = raw * 512 / sat;
    if (out >  512) out =  512;
    if (out < -512) out = -512;
    return out;
}

static void camServoController(const GamepadData* gp, VehicleState* state) {
    if (!state->camServoEnabled || !state->camGamepadEnabled) return;
    int dz  = (int)state->camStickDZ;
    int sat = (int)state->camStickSat;

    // Many budget gamepads have mechanical crosstalk between the analog
    // triggers (L2/R2) and the right stick Y axis: pressing a trigger shifts
    // axis_ry by a small amount (~30-60 units). When the motor is running,
    // double the deadzone to absorb that drift without the user needing to
    // retune camStickDZ. At rest, full sensitivity is restored.
    if (gp->brake > 20 || gp->throttle > 20) dz *= 2;

    setCamPan (scale_cam_stick(gp->axis_rx, dz, sat), state);
    setCamTilt(scale_cam_stick(gp->axis_ry, dz, sat), state);
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

    // RS click → centrar servos de cámara
    bool thumbR = (gp->buttons & GAMEPAD_BTN_THUMB_R) != 0;
    if (thumbR != btnState.thumbRLastState) btnState.thumbRPressed = true;
    else btnState.thumbRPressed = false;
    btnState.thumbRLastState = thumbR;
    if (btnState.thumbRPressed && btnState.thumbRLastState) {
        centerCamServos(state);
    }

    // Select/View → alternar modo hold de cámara y persistir
    bool sel = (gp->buttons & GAMEPAD_BTN_SELECT) != 0;
    if (sel != btnState.selectLastState) btnState.selectPressed = true;
    else btnState.selectPressed = false;
    btnState.selectLastState = sel;
    if (btnState.selectPressed && btnState.selectLastState) {
        state->camHoldMode = !state->camHoldMode;
        nvs_persist_bool("camHoldMode", state->camHoldMode);
    }
}

void handleGamepadButtons(VehicleState* state, ProgramManager* programManager) {
    // Data is pushed via hid_gamepad callbacks — no polling call needed.
    const GamepadData* gp = &state->myGamepads[0];
    if (gp->connected) {
        buttonController(gp, state, programManager);
    }
}

void handleGamepadMotion(VehicleState* state) {
    const GamepadData* gp = &state->myGamepads[0];
    if (!gp->connected) return;

    // Arbitraje "el último input activo gana": mientras la webapp/API tiene una
    // ventana de comando abierta, el gamepad cede el control. Conducción y
    // cámara se arbitran por separado (se puede manejar con el gamepad mientras
    // la webapp mueve la cámara, y viceversa).
    if (!state->apiActEnabled) {
        servoController(gp, state);
        motorController(gp, state);

        if (isRecording) {
            g_programManager->recordStep(lastMotor, lastSteer);
        }
    }

    if ((int32_t)(millis() - (uint32_t)state->apiCamActUntil) >= 0) {
        camServoController(gp, state);
    }
}
