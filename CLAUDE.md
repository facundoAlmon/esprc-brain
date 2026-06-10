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

Webapp-only video recording (`webapp/src/js/recorder.js`): browsers do NOT refresh a multipart MJPEG `<img>` when drawn to a canvas (drawImage always yields the first frame), so the recorder takes over the camera's single MJPEG connection while recording: clears `img.src`, `fetch()`es the stream (CORS ok — the mjpeg server sends `Access-Control-Allow-Origin: *`), splits JPEG frames by FFD8/FFD9 markers, paints them onto a hidden canvas (recorded via `captureStream` + MediaRecorder → webm/mp4 download) and keeps the on-screen `<img>` live with per-frame blob URLs. On stop it reconnects the normal `<img>` stream. Stream fallback URL uses port 81 directly (the port-80 302 redirect may not carry CORS).

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
| `src/actuators.cpp` | DC motor (MCPWM via L298N), steering servo (LEDC_TIMER_0, CH2), and camera pan/tilt servos (LEDC_TIMER_0, CH3/CH4) |
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

### Wi-Fi

AP mode disables Bluetooth scanning by design (BT/Wi-Fi coexistence workaround on ESP32). STA mode falls back to an open AP named `"ESP-RC-CAR"` after 10 failed connection retries.

### WebApp ↔ Firmware Contract

New features that cross the webapp/firmware boundary always require changes in both places:

1. Add the UI element and a `fetch`/WebSocket call in `webapp/src/script.js`
2. Add the endpoint handler in `src/webServerHandler.cpp`
3. Add any new state fields to `VehicleState` in `include/state.h`
4. Persist new config keys in `initPreferences()` / the save handler in `webServerHandler.cpp`
5. Run `npm run build` then `idf.py build` and flash

## Code Style

C++ follows Chromium style via `.clang-format` (120-column limit, 4-space indent). Run `clang-format` before committing firmware changes.
