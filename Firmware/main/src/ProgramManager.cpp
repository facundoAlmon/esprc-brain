#include "ProgramManager.h"
#include "actuators.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char* TAG = "ProgramManager";
static const char* NVS_PROGRAM_KEY = "program";
static const char* NVS_NS = "bl-car";

static inline uint32_t millis() { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

// NVS helpers used only in this file
static bool nvs_put_str(const char* key, const char* value) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_set_str(h, key, value) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h);
    return ok;
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

static bool nvs_has_key(const char* key) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    size_t sz = 0;
    bool found = (nvs_get_str(h, key, nullptr, &sz) == ESP_OK);
    nvs_close(h);
    return found;
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
    if (!nvs_has_key(NVS_PROGRAM_KEY)) {
        ESP_LOGI(TAG, "No program in NVS.");
        return;
    }

    std::string programString = nvs_get_str_or(NVS_PROGRAM_KEY, "");
    if (programString.empty()) {
        ESP_LOGW(TAG, "Program key exists but is empty.");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, programString);
    if (error) {
        ESP_LOGE(TAG, "Failed to parse program from NVS: %s", error.c_str());
        return;
    }

    parseProgramFromJson(doc.as<JsonArray>());
    ESP_LOGI(TAG, "Loaded %d actions from NVS.", _program.size());
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

    JsonDocument doc = getProgramAsJson();
    std::string programString;
    serializeJson(doc, programString);

    nvs_stats_t nvs_stats;
    if (nvs_get_stats(NULL, &nvs_stats) == ESP_OK) {
        size_t free_bytes = nvs_stats.free_entries * 32;
        if (programString.size() + 32 > free_bytes) {
            ESP_LOGE(TAG, "Not enough NVS space to save program!");
        }
    }

    if (nvs_put_str(NVS_PROGRAM_KEY, programString.c_str())) {
        ESP_LOGI(TAG, "Program saved to NVS. Size: %d bytes", (int)programString.size());
    } else {
        ESP_LOGE(TAG, "Failed to save program to NVS. Size: %d", (int)programString.size());
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
    if (nvs_has_key(NVS_PROGRAM_KEY)) {
        nvs_erase(NVS_PROGRAM_KEY);
        ESP_LOGI(TAG, "Program cleared from memory and NVS.");
    } else {
        ESP_LOGI(TAG, "Program cleared from memory.");
    }
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
