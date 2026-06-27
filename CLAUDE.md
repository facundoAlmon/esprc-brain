# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP-RC Brain is firmware for an ESP32-based RC car controller. It exposes a Wi-Fi web interface and Bluetooth gamepad support. The repo has two independently buildable sub-projects:

- **Firmware** (`Firmware/`) — C++ on pure ESP-IDF (no Arduino), compiled with `idf.py`
- **WebApp** (`Firmware/webapp/`) — HTML/CSS/JS, bundled by Gulp into a single `index.html` that gets embedded in the firmware binary

The firmware has **no local component dependencies** — all Bluetooth and peripheral support uses native ESP-IDF APIs.

## Stack & Managed Dependencies

| Dependency | Version | Source |
|------------|---------|--------|
| ESP-IDF | v6.0.1 | `/mnt/EVO_EXT4/DIY/esp-idf/.espressif/v6.0.1/esp-idf/` |
| espressif/led_strip | ^3.0.1 | component manager (`idf_component.yml`) |
| espressif/mdns | ^1.0.0 | component manager |
| bblanchon/arduinojson | ^7.4.2 | component manager |

## Bluetooth

Bluetooth gamepad support usa **ESP-IDF nativo** `esp_hid_host` + Bluedroid. Sin Bluepad32 ni BTstack.

- Supported controllers: **Xbox Series X/S** (BLE) y gamepads **HID estándar genéricos**
- **ESP32**: Classic BT + BLE (`HIDH_BTDM_MODE`)
- **ESP32-C6**: BLE-only (`HIDH_BLE_MODE`) — Xbox Series X/S funciona vía BLE. `CONFIG_BT_BLE_ENABLED=y` requerido en `sdkconfig.defaults.esp32c6`
- BT y C6: `bt`+`esp_hid` van en REQUIRES **incondicional** en CMakeLists — REQUIRES dinámico dentro de `if()` no propaga include paths en IDF CMake
- IDF v6.0 separó `driver` en sub-componentes: `esp_driver_gpio`, `esp_driver_ledc`, `esp_driver_mcpwm` deben estar en REQUIRES explícitamente (ya agregados)
- IDF v6.0 eliminó `driver/mcpwm.h` legacy — se usa la nueva API handle-based (`driver/mcpwm_prelude.h`) con timer→operator→comparator→generator
- IDF v6.0 eliminó el componente `json` (cJSON) — el proyecto usa solo ArduinoJSON, sin impacto
- `esp_hid_gap.h` necesita `#include "sdkconfig.h"` propio para que `HID_HOST_MODE` se evalúe correctamente al ser incluido desde C++
- AP mode deshabilita BT scanning (workaround coexistencia BT/Wi-Fi)
- Key files: `main/src/hid_gamepad.cpp`, `main/src/esp_hid_gap.c`, `main/include/hid_gamepad.h`

### Xbox Series X/S BLE HID report layout (confirmado desde hardware real)

Report ID 0x01, 16 bytes (report ID stripped por esp_hidh):

| Bytes | Campo | Detalle |
|-------|-------|---------|
| 0-1 | Left stick X | uint16 LE, centro=32767 |
| 2-3 | Left stick Y | uint16 LE, centro=32767 |
| 4-5 | Right stick X | uint16 LE, centro=32767 |
| 6-7 | Right stick Y | uint16 LE, centro=32767 |
| 8-9 | Left trigger | uint16, 0-1023 |
| 10-11 | Right trigger | uint16, 0-1023 |
| 12-13 | Botones + HAT | A=bit8 B=bit9 X=bit11 Y=bit12 LB=bit14 RB=bit15; HAT=bits3:0 (0=none,1=N,3=E,5=S,7=W) |
| 14 | Botones extra | View=bit2 Menu=bit3 LS=bit5 RS=bit6 |
| 15 | Botones extra | Capture=bit0 |

## Camera Servos (Pan/Tilt)

Two additional LEDC channels on `LEDC_TIMER_0` (same timer as steering servo — already configured at 50 Hz, 14-bit):

| Channel | Function | ESP32-C6 pin | ESP32 pin |
|---------|----------|-------------|-----------|
| `LEDC_CHANNEL_3` | Pan (left/right) | GPIO 4 | GPIO 14 |
| `LEDC_CHANNEL_4` | Tilt (up/down) | GPIO 5 | GPIO 27 |

Key implementation details:
- `setupCamServos()` is a no-op when `camServoEnabled == false` — channels are only configured on demand
- `camServoController()` in `gamepadHandler.cpp` doubles the deadzone when any trigger is >20 to absorb trigger→right-stick crosstalk common in budget gamepads
- WebSocket `/act` accepts `panAng` and `tiltAng` (integers, -512..512) for direct control from `joy3`
- Config backup/restore includes all servo NVS keys; config GET/POST buffer enlarged to 1024 bytes to fit them
- `prevCamEnabled` guard in `post_config_handler`: calls `setupCamServos()` only on first enable; otherwise just `centerCamServos()`

**Smooth servo update loop**: `setCamPan()`/`setCamTilt()` only store the angle target in `lastPanAngle`/`lastTiltAngle` — they do **not** write to LEDC directly. `updateCamServos(state)` is called every 15 ms from `main_task` and does all the work: integrates velocity (hold mode) or computes absolute position, then calls `cam_servo_write()`. This decouples the physical servo update rate from the WS frame rate (100 ms), eliminating the advance/pause stepping that was visible when the stick is deflected.

NVS keys (namespace `"bl-car"`): `camServoEn`, `camGamepadEn`, `panInvert`, `tiltInvert`, `camStickDZ`, `camStickSat`, `panCenterDeg`, `panLimitLDeg`, `panLimitRDeg`, `panMinUs`, `panMaxUs`, `tiltCenterDeg`, `tiltLimUpDeg`, `tiltLimDnDeg`, `tiltMinUs`, `tiltMaxUs`, `camHoldMode`, `camHoldSpeed`.

Hold mode (`camHoldMode`): stick/pad input is treated as angular velocity (`camHoldSpeed` °/s at full deflection); `updateCamServos()` integrates it every 15 ms (dt clamped to 0.1 s) — the servo moves continuously and smoothly as long as the stick is deflected, and holds its position when input returns to 0. `panPosDeg`/`tiltPosDeg` are kept in sync during absolute mode so switching to hold starts from the current physical position. `stopMotors()` skips `centerCamServos()` in hold mode.

WS/`/act` actions: `cam_hold_toggle` (flips `camHoldMode` and persists to NVS) and `cam_center` (calls `centerCamServos`). The webapp exposes hold/center buttons on all joystick views (`.cam-hold-btn`/`.cam-center-btn`, shown only when `camServoEnabled`).

**BT gamepad camera buttons**: RS click (`GAMEPAD_BTN_THUMB_R`) → `centerCamServos()`; Select/View (`GAMEPAD_BTN_SELECT`) → toggle `camHoldMode` + persist to NVS. Xbox: View = byte 14 bit 2 → `GAMEPAD_BTN_SELECT`. Generic HID: Select = bit 8 of the 16-bit button word (bit 0 of `d[7]`). NVS writes from `gamepadHandler.cpp` use a local `nvs_persist_bool()` helper (same open/set/commit/close pattern as `webServerHandler.cpp`).

`joy3.GetX/Y()` returns values relative to `internalRadius` (36% of canvas). At full deflection (handle at `externalRadius` = 68%), the value reaches ≈±189, not ±100 — always clamp `panAng`/`tiltAng` to ±512 before sending or processing. `CamPad.GetX/Y()` is correctly bounded to ±100.

**WS arbitration invariant**: `act_from_json` guards `setMotor`/`setSteer` behind `apiActEnabled` — neutral frames (speed=0, steer=0) must NOT execute when the priority window is closed or they override the BT gamepad. A neutral frame arriving inside an open window closes it immediately (no need to wait for the full `apiActMS` timeout).

**CamPad send callback**: `CamPad.setSendCallback(cb)` fires `cb(x, y)` on every `pointermove` (throttled to 30 ms) and on `pointerup` with (0,0). Used in `startActionLoop` to send camera-only WS frames `{ panAng, tiltAng, ms:150 }` for low-latency FPV control without touching motor/steer fields.

Webapp-only video recording (`webapp/src/js/recorder.js`): browsers do NOT refresh a multipart MJPEG `<img>` when drawn to a canvas (drawImage always yields the first frame), so the recorder takes over the camera's single MJPEG connection while recording: clears `img.src`, `fetch()`es the stream (CORS ok — the mjpeg server sends `Access-Control-Allow-Origin: *`), splits JPEG frames by FFD8/FFD9 markers, paints them onto a hidden canvas (recorded via `captureStream` + MediaRecorder → webm/mp4 download) and keeps the on-screen `<img>` live with per-frame blob URLs. On stop it reconnects the normal `<img>` stream. Stream fallback URL uses port 81 directly (the port-80 302 redirect may not carry CORS).

## Motor de Tracción: DC (L298N) o Brushless (ESC)

El tipo de motor se elige en runtime con `motorType` (NVS key `motorType`, namespace `"bl-car"`): `0` = motor DC vía L298N (MCPWM 20 kHz + 2 pines de dirección), `1` = brushless con ESC (señal PWM tipo servo).

`setMotor(speed 0-1023, forward, state)` sigue siendo la **API única** — gamepad, WS `/act`, ProgramManager y Kids mode no cambian. El despacho por `motorType` es interno a `actuators.cpp`:

- **DC**: lógica original (duty MCPWM en `pinMotorEn` + `pinMotorDir1/2`).
- **ESC**: `esc_write_us()` (espejo de `cam_servo_write`) emite un pulso en `ESC_CHANNEL` (`LEDC_CHANNEL_5`, mismo `LEDC_TIMER_0` de 50 Hz/14-bit que los servos). La señal **reusa `pinMotorEn`** (GPIO 19 en C6 / 25 en ESP32); los pines de dirección quedan sin uso. Mapeo: `speed=0`→`escCenterUs`; adelante→`escCenterUs..escMaxUs`; reversa→`escCenterUs..escMinUs`.

NVS keys: u32 `motorType`, `escMinUs` (1000), `escCenterUs` (1500), `escMaxUs` (2000), `escMinSpeed` (100, piso de throttle = umbral de arranque del brushless), `escMaxSpeed` (1023), `escMaxSpeedRev` (1023, límite de reversa separado del avance), `escBrakeMs` (200), `escRearmMs` (120), `gpThrottleDZ` (20), `gpSteerDZ` (20); bool `motorInvert` (false, invierte el sentido en DC y ESC). El ESC se arma en neutral al boot (`setupActuators`) y `stopMotors()` saca neutral en modo ESC.

**Salida diferida (`updateEsc`)**: en modo ESC, `setMotor()` solo guarda el target (`escTargetSpeed`/`escTargetForward`, runtime-only); la salida física la escribe `updateEsc(state)` **cada tick del main loop** (15 ms), igual patrón que `updateCamServos`. `updateEsc` aplica el escalado: avance `escMinSpeed..escMaxSpeed`, reversa `escMinSpeed..escMaxSpeedRev` (límite de reversa independiente; también capa el tirón de la fase de freno para que la reversa nunca supere ese límite). El escalado vale por igual para joystick virtual y BT (ambos pasan por `setMotor`).

**Modo Crawl (`escCrawlEnabled`)**: el SPMXSE1085 es sensorless → no gira por debajo de cierto RPM, y `escMaxSpeed` no puede bajar de ese mínimo continuo. El crawl **pulsa el throttle** para promediar velocidades menores: por debajo de `escMinSpeed` saca "patadas" de `escCrawlOnMs` (def 80) al umbral de arranque, separadas por un gap en neutral variable (`offMs = escCrawlOnMs*(escMinSpeed-d)/d`, capado a `escCrawlMaxOffMs` def 400). Helpers `crawl_kick_now()`/`crawl_off_us()` + statics `s_crawl_on`/`s_crawl_us` en `actuators.cpp`. Con crawl ON el mapeo de `updateEsc` es **sin piso** (`d = s*escMax/1023`): `d≥escMinSpeed`→continuo, `0<d<escMinSpeed`→patadas. Aplica a avance y a reversa (sólo en `ESC_PHASE_REVERSE`, ya cebada; el gap a neutral no desceba). Es a tirones por naturaleza. NVS: `escCrawlEn` (bool), `escCrawlOnMs`, `escCrawlOffMs`.

**Inversión de sentido (`motorInvert`)**: invierte adelante/reversa por software en DC (flip de `forward` en `setMotor`) y ESC (flip de `fwd` en `updateEsc`). Ojo: por la asimetría del ESC (avance inmediato, reversa con secuencia freno→neutral), invertir mueve esa secuencia al lado que quede como reversa; el arreglo "limpio" del sentido físico es intercambiar 2 de los 3 cables del brushless o el *Motor Rotation* del ESC (ítem #5).

**Reversa automática**: los ESC de auto interpretan el primer pulso de reversa como FRENO; recién tras pasar por neutral el siguiente es reversa real. `updateEsc` reproduce esa secuencia con una máquina de estados (`EscPhase`: NEUTRAL→FORWARD→BRAKE→REARM→REVERSE) usando timestamps configurables (`escBrakeMs` def 200 ms, `escRearmMs` def 120 ms, ajustables desde la webapp), así "tirar atrás" termina en reversa sin soltar/reapretar. Aplica tanto al joystick virtual como al BT (ambos pasan por `setMotor`). **Cebado de reversa** (`s_esc_rev_primed`): tras reversar el ESC queda cebado; mientras no se aplique avance, reversa→neutral→reversa entra **directo** (sin repetir el baile freno→neutral, que causaba un stutter "para y continúa"). Sólo el avance (fase FORWARD) resetea el cebado.

**Freno mantenido**: `brakeMotor(state, on)` + flag runtime `brakeActive`. En ESC saca pulso de freno (`escMinUs`) vía `updateEsc`; en DC frena activo el L298N (ambos pines de dirección en alto, EN a tope). WS actions `brake_on`/`brake_off` (botón `.brake-btn` por vista en la webapp, mantener-presionado). Un comando real de acelerador libera el freno.

**Umbrales del gamepad BT**: `gpThrottleDZ` (gatillo acelerador/freno) y `gpSteerDZ` (stick de dirección) reemplazan los `20` antes hardcodeados en `gamepadHandler.cpp` (`motorController`/`servoController`/`camServoController`).

**Cambiar `motorType` reinicia el ESP** (reconfigura el periférico de `pinMotorEn` entre MCPWM y LEDC) — `post_config_handler` reinicia tras responder, igual que `/api/pins`. Los `esc*Us` se aplican en caliente.

**Calibración** (`POST /api/esc/calibrate`, body `{"phase":"high|neutral|low|end"}`): `escCalibrateOutput()` mantiene un pulso de tope mientras `escCalibrating=true` (flag runtime, NO en NVS); ese flag hace que `updateEsc()` ceda la salida (no escribe el throttle normal). `"end"` vuelve a neutral y libera. Salvaguarda: si se olvidó "Finalizar", `updateEsc` auto-libera el flag a los 60 s (`ESC_CAL_TIMEOUT_US`) para no dejar el motor bloqueado. El ESC Spektrum SPMXSE1085 (Firma 85A) se calibra con su **botón SET**, orden **Neutral → Acelerador máx → Freno máx**. El asistente de la webapp vive en la pestaña config (`#escOptions`/`#escCalibrate`, wireado en `initMotorUI()` de `config.js`).

**Webapp config (modo ESC)**: al elegir Brushless se ocultan los sliders DC (`#dcMotorOptions`) y se muestran (`#escOptions`): Velocidad Máx/Mín (`escMaxSpeed`/`escMinSpeed`), sub-panel *Tiempos de reversa* (`escBrakeMs`/`escRearmMs`) y *Calibración de pulsos* avanzado (`esc*Us`). El `<select id="motorType">` serializa como string — `saveConfig()` hace `parseInt` antes de enviar (ArduinoJson no coacciona string→int).

> **Comportamiento del ESC (no es firmware)**: el SPMXSE1085 viene de fábrica en *Forward/Reverse w/Brake* (reversa habilitada). Los ESC de auto exigen el "doble toque" (1er pulso atrás = freno → neutral → 2º atrás = reversa); la máquina de estados de `updateEsc` lo automatiza. Si el motor "solo va adelante y frena", o el Running Mode quedó en *Forward w/Brake* (reprogramar con el botón SET: ítem #4 → opción 1, o reset de fábrica manteniendo SET 5 s), o faltó hacer el doble toque. `escBrakeMs`/`escRearmMs` en 0 hacen la reversa casi instantánea.

## Partition Table

Uses a custom `Firmware/partitions.csv` with a **dual-OTA layout** for 8 MB flash:
- `nvs` (0x6000), `otadata` (0x2000), `phy_init` (0x1000)
- `ota_0` at 0x20000 (3.94 MB) — active slot at first flash
- `ota_1` at 0x410000 (3.94 MB) — receives OTA updates

There is **no factory partition**. The first USB flash places the app in `ota_0`; subsequent OTA writes alternate between `ota_0` and `ota_1`. Current binary is ~1.4 MB (ESP32) / ~1.0 MB (C6), leaving ~65% headroom per slot.

If the chip only has 4 MB flash, switch `CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y` → `_4MB=y` in `sdkconfig.defaults` and shrink each slot to 0x1F0000 (start ota_1 at 0x210000).

After editing `partitions.csv`, delete `sdkconfig` to regenerate the cached layout, then `idf.py build` and reflash via USB once.

## OTA (Firmware Update)

Implemented via the existing web server (no extra components beyond ESP-IDF's `app_update`).

- Upload file: **`Firmware/build/esprc_brain.bin`** (app binary only — bootloader and partition table are USB-only)
- `GET /api/ota/info` — returns running/boot/next partition info + `esp_app_desc_t` (version, build date, IDF version)
- `POST /api/ota` — body is the raw `.bin` (binary stream). Receives in 4 KB chunks, writes with `esp_ota_write`, calls `esp_ota_set_boot_partition`, sends `{"status":"ok"}`, then `esp_restart()`. Validates the 0xE9 magic byte before touching flash. Stops `ProgramManager`, sequence task, and motors before writing.
- WebApp tab "ESP32 Manage" shows the OTA section with file picker, progress bar and reboot confirmation (`webapp/src/js/ota.js`).

Rollback: `esp_ota_mark_app_valid_cancel_rollback()` is called in `main_task` after all subsystems start. It's a no-op unless `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` (off by default).

OTA is only possible over Wi-Fi (AP or STA).

## Firmware Commands

First-time setup on a fresh clone (run once per target). The IDF v6.0.1 environment must be active — the shell does **not** persist between sessions, so export before every build session:

```bash
# Activate IDF v6.0.1 (run in every new terminal session)
export IDF_PATH=/mnt/EVO_EXT4/DIY/esp-idf/.espressif/v6.0.1/esp-idf
export IDF_PYTHON_ENV_PATH=/home/falmon/.espressif/python_env/idf6.0_py3.12_env
export ESP_IDF_VERSION=6.0.1
export PATH="$IDF_PATH/tools:/home/falmon/.espressif/tools/riscv32-esp-elf/esp-15.2.0_20251204/riscv32-esp-elf/bin:/home/falmon/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/bin:$IDF_PYTHON_ENV_PATH/bin:$PATH"

cd Firmware
idf.py set-target esp32c6   # or: set-target esp32
```

After switching IDF versions or changing target, delete `sdkconfig` to force regeneration:
```bash
rm -f Firmware/sdkconfig
```

All `idf.py` commands must be run from `Firmware/`:

```bash
idf.py menuconfig          # Configure ESP-IDF options (board, peripherals)
idf.py build               # Compile
idf.py -p /dev/ttyUSB0 flash monitor  # Flash and open serial console
idf.py -p /dev/ttyUSB0 monitor        # Serial console only
```

Supported targets: `esp32` and `esp32c6`. Pins are selected at compile time via `pins.h` using `CONFIG_IDF_TARGET_*` macros.

`sdkconfig` is gitignored — make persistent config changes in `sdkconfig.defaults` (all targets) or `sdkconfig.defaults.esp32c6` (C6-specific). `managed_components/` is also gitignored; it is populated automatically on first `idf.py build` from `idf_component.yml`.

## WebApp Commands

All commands run from `Firmware/webapp/`:

```bash
npm install         # Install dependencies (first time only)
npm run build       # Bundle src/ → build/ → copies index.html to Firmware/main/
npm run serve       # Serves ./src/ at http://localhost:8080 (requires global: npm i -g serve)
npm run clean       # Delete build artifacts
```

After `npm run build`, re-run `idf.py build` and flash to apply webapp changes to the device.

`npm run serve` uses source files directly — change the API/WebSocket URL fields in the UI to point at your ESP32's IP.

The Gulp pipeline: Rollup bundles `script.js` → `gulp-inline-source` inlines CSS and JS into `index.html` → `htmlmin` minifies → output copied to `Firmware/main/index.html`. CMake embeds this file into the firmware binary via `EMBED_FILES`.

### CSS gotcha — imágenes fullscreen en mobile landscape

Para `<img>` dentro de un flex container fullscreen, usar `width:100%; height:100%; object-fit:contain` + `min-height:0` en el contenedor. `max-width/max-height:100%` falla en landscape mobile porque el browser no resuelve `max-height` correctamente contra un flex item sin `min-height:0`.

## Architecture

### State

`include/state.h` defines `VehicleState` — a single global struct that is the source of truth for all runtime state (motor/servo config, LED groups, Wi-Fi settings, gamepad state, API action state). Every module receives a `VehicleState*` pointer. `VehicleState` is instantiated in `sketch.cpp`.

Configuration is persisted to ESP32 NVS under the namespace `"bl-car"`. LED config is stored as a JSON string.

### Firmware Modules

| File | Responsibility |
|------|---------------|
| `main.c` | Entry point; calls `app_task_start()` which creates the main FreeRTOS task |
| `src/sketch.cpp` | `main_task`: nvs_flash_init, initPreferences, WiFi, actuators, gamepad, web server, main loop (15 ms) |
| `src/actuators.cpp` | Motor de tracción — DC (MCPWM via L298N) o brushless ESC (LEDC CH5, señal en `pinMotorEn`), según `motorType`; steering servo (LEDC_TIMER_0, CH2); camera pan/tilt servos (LEDC_TIMER_0, CH3/CH4); calibración ESC |
| `src/gamepadHandler.cpp` | Gamepad callbacks; maps axes/buttons to motor/steer/lights/cam servos |
| `src/ledStripHandler.cpp` | WS2812B strip via RMT encoder; maps `LedFunction` groups to physical LEDs |
| `src/webServerHandler.cpp` | `esp_http_server` REST API + WebSocket; serves the embedded `index.html` |
| `src/ProgramManager.cpp` | Records/plays back movement sequences; persists programs to NVS |
| `src/nvs_prefs.cpp` | NVS wrapper replacing Arduino Preferences (`NvsPrefs` class) |
| `src/hid_gamepad.cpp` | BT init, scan loop, Xbox Series X/S HID parser, generic HID parser (ESP32 only) |
| `src/esp_hid_gap.c` | BT controller + Bluedroid init, GAP scanning (adapted from IDF example) (ESP32 only) |
| `src/hid_gamepad_stub.cpp` | No-op stubs for non-ESP32 targets |
| `src/dns_server.c` | Captive DNS for AP mode |
| `src/state.cpp` | State utility helpers |

### Main Loop

`main_task` runs a 15 ms non-blocking interval loop. Priority order each tick:

1. `handleGamepadButtons()` — always processed (lights, recording, program control)
2. `handleGamepadMotion()` — skipped when `ProgramManager::isRunning()`. WS/BT arbitration ("last active input wins"): drive control is skipped while `apiActEnabled` (open only by non-neutral WS commands), cam servo control while `millis() < apiCamActUntil`. The webapp action loop only transmits while there is real input (plus one neutral frame on release; always transmits while recording a program), so an idle webapp never locks out the gamepad.
3. `updateCamServos()` — always called; writes the physical servo position every tick (smooth interpolation regardless of WS/gamepad rate)
4. API action timeout check (`vehicleState.apiActEnabled`)
5. `programManager.loop()` — advances the active sequence
6. `handleLedStrip()` — updates the LED strip

### WebApp JS Modules

`script.js` is the Rollup entry; it re-exports from `js/index.js`. Feature-level logic lives in:

| Module | Purpose |
|--------|---------|
| `js/api.js` | `fetchAPI()` helper and WebSocket connect/send |
| `js/state.js` | Shared `state` and `elements` objects |
| `js/ui.js` | Tab navigation, event listeners, UI helpers; `enterFpv()`/`leaveFpv()` move `#camImg` in/out of FPV container; `handleCamHoldToggle()`/`handleCamCenter()` |
| `js/joystick.js` | Virtual joystick rendering and action loop; `joy3` sends `panAng`/`tiltAng`; `CamPad` class for invisible FPV overlay touch control; `joyFpv` for FPV drive joystick |
| `js/recorder.js` | Webapp-only MJPEG video recorder: takes over the camera's single connection via `fetch()`, parses FFD8/FFD9 markers, renders frames to a hidden canvas via `createImageBitmap`, records with MediaRecorder (webm/mp4), keeps on-screen `<img>` live with per-frame blob URLs |
| `js/config.js` | Car config GET/POST; `initCamServoUI()` wires toggle + servo-type presets; reads `camHoldMode` and calls `updateCamHoldButtons()` |
| `js/lights.js` | Headlight/turn-signal/hazard UI — generic loop covers Joy A, Joy B, and FPV tabs |
| `js/leds.js` | LED group configuration UI |
| `js/program.js` | Sequence editor and program execution |
| `js/kidMode.js` | Kids block-programming UI |
| `js/language.js` / `translations.js` | i18n |

### Communication

- **WebSocket** (`/ws`) — real-time joystick/motor commands; payload includes `panAng` (-512..512) and `tiltAng` (-512..512) when cam servos are enabled (via `sendWsAction()`)
- **REST API** — configuration reads/writes, program upload/download, system actions (restart, factory reset)
- **NVS** — all configuration persisted across reboots

### REST API Endpoints

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/` | Serves the embedded `index.html` |
| GET/POST | `/config` | Car parameters (speed, servo, BT, etc.) |
| GET/POST | `/wifi` | Wi-Fi mode, SSID, password |
| GET/POST | `/api/leds` | LED group configuration |
| POST | `/act` | One-shot timed movement command |
| POST | `/manage` | System actions (restart, factory reset) |
| GET | `/ws` | WebSocket upgrade |
| GET/POST | `/api/program` | Load/fetch movement program (JSON) |
| GET | `/api/program/run` | Start program execution |
| GET | `/api/program/stop` | Stop program execution |
| GET | `/api/program/clear` | Clear program from NVS |
| GET | `/api/recording/start` | Begin recording from joystick input |
| GET | `/api/recording/stop` | Stop recording, save to program |
| GET | `/api/config/backup` | Export full config as JSON |
| POST | `/api/config/restore` | Restore config from JSON |
| POST | `/api/sequence` | Run ad-hoc sequence (Kids mode) |
| GET | `/api/sequence/stop` | Stop ad-hoc sequence |
| GET | `/api/ota/info` | Running/boot/next partition + app description |
| POST | `/api/ota` | Upload `esprc_brain.bin` (binary body); reboots on success |
| POST | `/api/esc/calibrate` | Calibración del ESC brushless: `{"phase":"high\|neutral\|low\|end"}` (solo si `motorType==1`) |

### Wi-Fi

AP mode disables Bluetooth scanning by design (BT/Wi-Fi coexistence workaround on ESP32). STA mode falls back to an open AP named `"ESP-RC-CAR"` after 10 failed connection retries.

### WebApp ↔ Firmware Contract

New features that cross the webapp/firmware boundary always require changes in both places:

1. Add the UI element and a `fetch`/WebSocket call in `webapp/src/script.js`
2. Add the endpoint handler in `src/webServerHandler.cpp`
3. Add any new state fields to `VehicleState` in `include/state.h`
4. Persist new config keys in `initPreferences()` / the save handler in `webServerHandler.cpp`
5. Run `npm run build` then `idf.py build` and flash

## GPIO Pins (configurables en runtime)

Los GPIO viven en `VehicleState.pin*` y se cargan desde NVS en `initPreferences()` con defaults compile-time de `pins.h`:

| Campo state      | NVS key       | Default macro           |
|------------------|---------------|-------------------------|
| `pinLedStrip`    | `pinLedStrip` | `PIN_LED_STRIP_DEFAULT` |
| `pinMotorEn`     | `pinMotorEn`  | `PIN_MOTOR_EN_DEFAULT`  |
| `pinMotorDir1`   | `pinMotorDir1`| `PIN_MOTOR_DIR1_DEFAULT`|
| `pinMotorDir2`   | `pinMotorDir2`| `PIN_MOTOR_DIR2_DEFAULT`|
| `pinSteerServo`  | `pinSteer`    | `PIN_STEER_DEFAULT`     |
| `pinPanServo`    | `pinPan`      | `PIN_PAN_DEFAULT`       |
| `pinTiltServo`   | `pinTilt`     | `PIN_TILT_DEFAULT`      |

`pins.h` solo define macros `PIN_*_DEFAULT` y `PIN_CHIP_TYPE` — **no declara variables runtime**. Canales LEDC son fijos: `STEER_SERVO_CHANNEL`=CH2, `PAN_SERVO_CHANNEL`=CH3, `TILT_SERVO_CHANNEL`=CH4, `ESC_CHANNEL`=CH5 (señal del ESC brushless sobre `pinMotorEn`).

`setupActuators(VehicleState*)` — recibe state pointer (firma cambiada; versiones anteriores no tenían argumento).

REST: `GET /api/pins` (current+defaults+chipType) · `POST /api/pins` (save+reboot) · `POST /api/pins/reset` (erase NVS keys+reboot → next boot usa compile-time defaults).

OTA safety: si las NVS keys no existen (primer boot post-OTA), `getUInt(key, DEFAULT)` retorna el default compile-time — sin pérdida de config.

### Patrón para agregar endpoints en webServerHandler.cpp

1. Incrementar `max_uri_handlers` en `startServer()`
2. Agregar forward declaration junto a las existentes
3. Implementar handler
4. Registrar URI con `httpd_register_uri_handler`
5. Agregar keys en `get_config_backup_handler` + `post_config_restore_handler`

NVS keys: máximo 15 caracteres — usar abreviaturas (`pinSteer` no `pinSteerServo`).

## Code Style

C++ follows Chromium style via `.clang-format` (120-column limit, 4-space indent). Run `clang-format` before committing firmware changes.
