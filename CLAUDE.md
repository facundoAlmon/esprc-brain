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
| ESP-IDF | v5.5.4 | `/mnt/EVO_EXT4/DIY/esp-idf/.espressif/v5.5.4/esp-idf/` |
| espressif/led_strip | ^3.0.1 | component manager (`idf_component.yml`) |
| espressif/mdns | ^1.0.0 | component manager |
| bblanchon/arduinojson | ^7.4.2 | component manager |

ESP-IDF source also at `/mnt/EVO_EXT4/DIY/esp-idf/.espressif/v6.0.1/`.

## Bluetooth

Bluetooth gamepad support usa **ESP-IDF nativo** `esp_hid_host` + Bluedroid. Sin Bluepad32 ni BTstack.

- Supported controllers: **Xbox Series X/S** (BLE) y gamepads **HID estándar genéricos**
- **ESP32**: Classic BT + BLE (`HIDH_BTDM_MODE`)
- **ESP32-C6**: BLE-only (`HIDH_BLE_MODE`) — Xbox Series X/S funciona vía BLE. `CONFIG_BT_BLE_ENABLED=y` requerido en `sdkconfig.defaults.esp32c6`
- BT y C6: `bt`+`esp_hid` van en REQUIRES **incondicional** en CMakeLists — REQUIRES dinámico dentro de `if()` no propaga include paths en IDF CMake
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

First-time setup on a fresh clone (run once per target):

```bash
cd Firmware
idf.py set-target esp32c6   # or: set-target esp32
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

## Architecture

### State

`include/state.h` defines `VehicleState` — a single global struct that is the source of truth for all runtime state (motor/servo config, LED groups, Wi-Fi settings, gamepad state, API action state). Every module receives a `VehicleState*` pointer. `VehicleState` is instantiated in `sketch.cpp`.

Configuration is persisted to ESP32 NVS under the namespace `"bl-car"`. LED config is stored as a JSON string.

### Firmware Modules

| File | Responsibility |
|------|---------------|
| `main.c` | Entry point; calls `app_task_start()` which creates the main FreeRTOS task |
| `src/sketch.cpp` | `main_task`: nvs_flash_init, initPreferences, WiFi, actuators, gamepad, web server, main loop (15 ms) |
| `src/actuators.cpp` | DC motor (MCPWM via L298N) and steering servo (LEDC direct, LEDC_TIMER_0, 50 Hz, 14-bit) |
| `src/gamepadHandler.cpp` | Gamepad callbacks; maps axes/buttons to motor/steer/lights |
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
2. `handleGamepadMotion()` — skipped when `ProgramManager::isRunning()`
3. API action timeout check (`vehicleState.apiActEnabled`)
4. `programManager.loop()` — advances the active sequence
5. `handleLedStrip()` — updates the LED strip

### WebApp JS Modules

`script.js` is the Rollup entry; it re-exports from `js/index.js`. Feature-level logic lives in:

| Module | Purpose |
|--------|---------|
| `js/api.js` | `fetchAPI()` helper and WebSocket connect/send |
| `js/state.js` | Shared `state` and `elements` objects |
| `js/ui.js` | Tab navigation, event listeners, UI helpers |
| `js/joystick.js` | Virtual joystick rendering and action loop |
| `js/config.js` | Car config GET/POST |
| `js/lights.js` | Headlight/turn-signal/hazard UI |
| `js/leds.js` | LED group configuration UI |
| `js/program.js` | Sequence editor and program execution |
| `js/kidMode.js` | Kids block-programming UI |
| `js/language.js` / `translations.js` | i18n |

### Communication

- **WebSocket** (`/ws`) — real-time joystick/motor commands; payload: `{ "action": <value> }` (via `sendWsAction()`)
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
