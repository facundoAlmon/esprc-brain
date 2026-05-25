# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP-RC Brain is firmware for an ESP32-based RC car controller. It exposes a Wi-Fi web interface and Bluetooth gamepad support. The repo has two independently buildable sub-projects:

- **Firmware** (`Firmware/`) — C++ on ESP-IDF + Arduino component, compiled with `idf.py`
- **WebApp** (`Firmware/webapp/`) — HTML/CSS/JS, bundled by Gulp into a single `index.html` that gets embedded in the firmware binary

## Component Versions & IDF Compatibility

| Component | Version | Location |
|-----------|---------|----------|
| arduino-esp32 | 3.3.8 | `Firmware/components/arduino/` (local, not managed) |
| bluepad32 | 4.2.0 | `Firmware/components/bluepad32/` (local) |

ESP-IDF versions installed locally: `v5.5.4` and `v6.0.1` at `/mnt/EVO_EXT4/DIY/esp-idf/.espressif/`.

To update the arduino component: `rm -rf Firmware/components/arduino && git clone --branch X.X.X --depth 1 https://github.com/espressif/arduino-esp32.git Firmware/components/arduino`

When IDF version conflicts cause build errors, update the component — don't patch individual files.

## Partition Table

Uses a custom `Firmware/partitions.csv` (not the built-in `SINGLE_APP_LARGE`):
- factory app: 0x1C0000 (1792 KB) — required after arduino-esp32 3.3.8 grew the binary past the 1500 KB default
- nvs: 0x6000, phy_init: 0x1000

If the binary overflows the factory partition again, increase `factory` size in `partitions.csv` and delete `sdkconfig` to regenerate.

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
| `main.c` | Bluepad32 / BTstack entry point; calls `btstack_run_loop_execute()` |
| `src/sketch.cpp` | Arduino `setup()` / `loop()`; initialises all subsystems, manages Wi-Fi (AP and STA modes) |
| `src/actuators.cpp` | DC motor (LEDC PWM via L298N enable/direction pins) and steering servo |
| `src/gamepadHandler.cpp` | Bluepad32 gamepad callbacks; maps axes/buttons to motor/steer/lights |
| `src/ledStripHandler.cpp` | WS2812B strip via RMT encoder; maps `LedFunction` groups to physical LEDs |
| `src/webServerHandler.cpp` | `esp_http_server` REST API + WebSocket; serves the embedded `index.html` |
| `src/ProgramManager.cpp` | Records/plays back movement sequences; persists programs to NVS |
| `src/state.cpp` | State utility helpers |

### Main Loop

`loop()` runs on a 15 ms non-blocking interval. Priority order each tick:

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
