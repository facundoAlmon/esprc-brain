#include "ProgramManager.h"
#include "actuators.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <vector>

static const char* TAG = "ProgramManager";
static const char* NVS_PROGRAM_KEY = "program";      // legacy JSON key
static const char* NVS_PROGRAM_BIN_KEY = "prog_bin"; // binary format key
static const char* NVS_NS = "bl-car";
static const uint8_t BIN_FORMAT_VERSION = 1;

// Binary on-disk layout per action: 9 bytes packed.
// Reduces storage ~7x vs JSON (65 bytes/action → 9 bytes/action).
#pragma pack(push, 1)
struct ProgramActionBin {
    uint8_t  type;
    int16_t  motorSpeed;
    int16_t  steerAngle;
    uint32_t duration_ms;
};
#pragma pack(pop)

static inline uint32_t millis() { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

static bool nvs_put_blob(const char* key, const void* data, size_t len) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_set_blob(h, key, data, len) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h);
    return ok;
}

static bool nvs_get_blob(const char* key, std::vector<uint8_t>& out) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    size_t sz = 0;
    if (nvs_get_blob(h, key, nullptr, &sz) != ESP_OK || sz == 0) { nvs_close(h); return false; }
    out.resize(sz);
    esp_err_t err = nvs_get_blob(h, key, out.data(), &sz);
    nvs_close(h);
    return err == ESP_OK;
}

static std::string nvs_get_str_or(const char* key, const char* def) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return def;
    size_t sz = 0;
    if (nvs_get_str(h, key, nullptr, &sz) != ESP_OK || sz == 0) { nvs_close(h); return def; }
    std::string result(sz, '\0');
    nvs_get_str(h, key, &result[0], &sz);
    nvs_close(h);
    if (!result.empty() && result.back() == '\0') result.pop_back();
    return result;
}


static bool nvs_erase(const char* key) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t err = nvs_erase_key(h, key);
    nvs_commit(h);
    nvs_close(h);
    return (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND);
}

// ---- ProgramManager implementation ----

ProgramManager::ProgramManager(VehicleState* state) : _state(state) {}

void ProgramManager::loadProgram(const JsonArray& programJson) {
    clearProgram();
    parseProgramFromJson(programJson);
    ESP_LOGI(TAG, "Loaded %d actions from payload.", _program.size());
    saveProgramToNVS();
}

void ProgramManager::loadProgramFromNVS() {
    // Try binary format first (new, efficient)
    std::vector<uint8_t> blob;
    if (nvs_get_blob(NVS_PROGRAM_BIN_KEY, blob) && blob.size() > 1) {
        if (blob[0] != BIN_FORMAT_VERSION) {
            ESP_LOGW(TAG, "Unknown binary program version %d, skipping.", blob[0]);
        } else {
            size_t count = (blob.size() - 1) / sizeof(ProgramActionBin);
            auto* actions = reinterpret_cast<const ProgramActionBin*>(blob.data() + 1);
            _program.clear();
            _program.reserve(count);
            for (size_t i = 0; i < count; i++) {
                ProgrammedAction a;
                a.type        = static_cast<ProgramActionType>(actions[i].type);
                a.motorSpeed  = actions[i].motorSpeed;
                a.steerAngle  = actions[i].steerAngle;
                a.duration_ms = actions[i].duration_ms;
                _program.push_back(a);
            }
            ESP_LOGI(TAG, "Loaded %d actions from NVS (binary, %d bytes).",
                     (int)_program.size(), (int)blob.size());
            return;
        }
    }

    // Fallback: legacy JSON format — load once, then migrate to binary
    std::string programString = nvs_get_str_or(NVS_PROGRAM_KEY, "");
    if (programString.empty()) {
        ESP_LOGI(TAG, "No program in NVS.");
        return;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, programString);
    if (error) {
        ESP_LOGE(TAG, "Failed to parse legacy program JSON: %s", error.c_str());
        return;
    }
    parseProgramFromJson(doc.as<JsonArray>());
    ESP_LOGI(TAG, "Loaded %d actions from NVS (JSON legacy). Migrating to binary.", (int)_program.size());
    saveProgramToNVS(); // converts and erases old JSON key
}

void ProgramManager::parseProgramFromJson(const JsonArray& programJson) {
    _program.clear();
    for (JsonObject actionJson : programJson) {
        ProgrammedAction action;
        const char* typeStr = actionJson["action"];

        if (strcmp(typeStr, "move") == 0) {
            action.type = ProgramActionType::MOVE_STEER;
            action.motorSpeed = actionJson["motorSpeed"] | 0;
            action.steerAngle = actionJson["steerAngle"] | 0;
        } else if (strcmp(typeStr, "wait") == 0) {
            action.type = ProgramActionType::WAIT;
        } else if (strcmp(typeStr, "lights_cycle") == 0) {
            action.type = ProgramActionType::LIGHTS_CYCLE;
        } else if (strcmp(typeStr, "hazards_toggle") == 0) {
            action.type = ProgramActionType::HAZARDS_TOGGLE;
        } else if (strcmp(typeStr, "right_turn_toggle") == 0) {
            action.type = ProgramActionType::RIGHT_TURN_TOGGLE;
        } else if (strcmp(typeStr, "left_turn_toggle") == 0) {
            action.type = ProgramActionType::LEFT_TURN_TOGGLE;
        } else {
            ESP_LOGW(TAG, "Unknown action type: %s", typeStr);
            continue;
        }

        action.duration_ms = actionJson["duration"] | 100;
        _program.push_back(action);
    }
}

void ProgramManager::saveProgramToNVS() {
    if (_program.empty()) { clearProgram(); return; }

    size_t count = _program.size();
    // Blob layout: 1 byte version header + count * ProgramActionBin
    size_t blobSize = 1 + count * sizeof(ProgramActionBin);
    std::vector<uint8_t> blob(blobSize);
    blob[0] = BIN_FORMAT_VERSION;
    auto* dst = reinterpret_cast<ProgramActionBin*>(blob.data() + 1);
    for (size_t i = 0; i < count; i++) {
        dst[i].type        = static_cast<uint8_t>(_program[i].type);
        dst[i].motorSpeed  = static_cast<int16_t>(_program[i].motorSpeed);
        dst[i].steerAngle  = static_cast<int16_t>(_program[i].steerAngle);
        dst[i].duration_ms = _program[i].duration_ms;
    }

    if (nvs_put_blob(NVS_PROGRAM_BIN_KEY, blob.data(), blobSize)) {
        ESP_LOGI(TAG, "Program saved to NVS (binary). %d actions, %d bytes.", (int)count, (int)blobSize);
        nvs_erase(NVS_PROGRAM_KEY); // remove legacy JSON key if present
    } else {
        ESP_LOGE(TAG, "Failed to save program to NVS. %d actions, %d bytes.", (int)count, (int)blobSize);
    }
}

void ProgramManager::startProgram(int iterations) {
    if (_program.empty() || _isRunning) {
        ESP_LOGW(TAG, "Cannot start program. Empty or already running.");
        return;
    }
    _isRunning = true;
    _currentActionIndex = 0;
    _actionStartTime = millis();
    _totalIterations = iterations;
    _currentIteration = 0;
    ESP_LOGI(TAG, "Starting program with %s iterations.",
             _totalIterations == -1 ? "infinite" : std::to_string(_totalIterations).c_str());
    executeAction(_program[_currentActionIndex]);
}

void ProgramManager::stopProgram() {
    if (!_isRunning) return;
    _isRunning = false;
    _totalIterations = 0;
    _currentIteration = 0;
    stopAllActions();
    ESP_LOGI(TAG, "Program stopped.");
}

void ProgramManager::clearProgram() {
    stopProgram();
    _program.clear();
    nvs_erase(NVS_PROGRAM_BIN_KEY);
    nvs_erase(NVS_PROGRAM_KEY);
    ESP_LOGI(TAG, "Program cleared.");
}

bool ProgramManager::isRecording() const { return _isRecording; }

void ProgramManager::startRecording() {
    if (_isRunning) { ESP_LOGW(TAG, "Cannot start recording, program is running."); return; }
    clearProgram();
    _isRecording = true;
    _lastActionRecordTime = millis();
    _lastRecordedAction = { ProgramActionType::MOVE_STEER, 0, 0, 0 };
    ESP_LOGI(TAG, "Recording started.");
}

void ProgramManager::stopRecording() {
    if (!_isRecording) return;
    _isRecording = false;
    uint32_t now = millis();
    uint32_t duration = now - _lastActionRecordTime;
    if (duration > 5) {
        _lastRecordedAction.duration_ms = duration;
        _program.push_back(_lastRecordedAction);
    }
    ESP_LOGI(TAG, "Recording stopped. Program has %d steps.", (int)_program.size());
    saveProgramToNVS();
}

void ProgramManager::recordStep(int motorSpeed, int steerAngle) {
    if (!_isRecording) return;
    if (motorSpeed != _lastRecordedAction.motorSpeed || steerAngle != _lastRecordedAction.steerAngle) {
        uint32_t now = millis();
        uint32_t duration = now - _lastActionRecordTime;
        if (duration > 5) {
            _lastRecordedAction.duration_ms = duration;
            _program.push_back(_lastRecordedAction);
            _lastRecordedAction = { ProgramActionType::MOVE_STEER, 0, motorSpeed, steerAngle };
            _lastActionRecordTime = now;
        }
    }
}

void ProgramManager::loop() {
    if (!_isRunning) return;

    uint32_t now = millis();
    if (now - _actionStartTime >= _program[_currentActionIndex].duration_ms) {
        uint32_t last_duration = _program[_currentActionIndex].duration_ms;
        _currentActionIndex++;

        if (_currentActionIndex >= _program.size()) {
            _currentIteration++;
            bool should_continue = (_totalIterations == -1) || (_currentIteration < _totalIterations);
            if (should_continue) {
                _currentActionIndex = 0;
                _actionStartTime += last_duration;
                executeAction(_program[_currentActionIndex]);
            } else {
                ESP_LOGI(TAG, "Finished all %d iterations.", _totalIterations);
                stopProgram();
            }
        } else {
            _actionStartTime += last_duration;
            executeAction(_program[_currentActionIndex]);
        }
    }
}

bool ProgramManager::isRunning() const { return _isRunning; }

JsonDocument ProgramManager::getProgramAsJson() {
    JsonDocument doc;
    JsonArray programJson = doc.to<JsonArray>();
    for (const auto& action : _program) {
        JsonObject actionJson = programJson.createNestedObject();
        actionJson["duration"] = action.duration_ms;
        switch (action.type) {
            case ProgramActionType::MOVE_STEER:
                actionJson["action"] = "move";
                actionJson["motorSpeed"] = action.motorSpeed;
                actionJson["steerAngle"] = action.steerAngle;
                break;
            case ProgramActionType::WAIT:          actionJson["action"] = "wait"; break;
            case ProgramActionType::LIGHTS_CYCLE:  actionJson["action"] = "lights_cycle"; break;
            case ProgramActionType::HAZARDS_TOGGLE:actionJson["action"] = "hazards_toggle"; break;
            case ProgramActionType::RIGHT_TURN_TOGGLE: actionJson["action"] = "right_turn_toggle"; break;
            case ProgramActionType::LEFT_TURN_TOGGLE:  actionJson["action"] = "left_turn_toggle"; break;
        }
    }
    return doc;
}

void ProgramManager::executeAction(const ProgrammedAction& action) {
    ESP_LOGI(TAG, "Executing action index %d", (int)_currentActionIndex);
    stopAllActions();
    switch (action.type) {
        case ProgramActionType::MOVE_STEER:
            setMotor(action.motorSpeed, action.motorSpeed >= 0, _state);
            setSteer(action.steerAngle, _state);
            break;
        case ProgramActionType::LIGHTS_CYCLE:
            _state->luces++; if (_state->luces > 3) _state->luces = 0; break;
        case ProgramActionType::HAZARDS_TOGGLE:
            _state->baliza = !_state->baliza; break;
        case ProgramActionType::RIGHT_TURN_TOGGLE:
            _state->giroDerecho = !_state->giroDerecho; break;
        case ProgramActionType::LEFT_TURN_TOGGLE:
            _state->giroIzquierdo = !_state->giroIzquierdo; break;
        case ProgramActionType::WAIT: break;
    }
}

void ProgramManager::stopAllActions() {
    setMotor(0, true, _state);
    setSteer(0, _state);
}
