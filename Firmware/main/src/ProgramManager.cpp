#include "ProgramManager.h"
#include "actuators.h" // For setMotor, setSteer
#include <esp_log.h>
#include "nvs_flash.h"
#include "nvs.h"

// The single global instance of Preferences
extern Preferences preferences;

static const char* TAG = "ProgramManager";
static const char* NVS_PROGRAM_KEY = "program";

ProgramManager::ProgramManager(VehicleState* state) : _state(state) {}

void ProgramManager::loadProgram(const JsonArray& programJson) {
    clearProgram(); // Clear current program in memory and NVS
    parseProgramFromJson(programJson);
    ESP_LOGI(TAG, "Loaded %d actions into program from payload.", _program.size());
    saveProgramToNVS(); // Save the new program to NVS
}

void ProgramManager::loadProgramFromNVS() {
    if (!preferences.isKey(NVS_PROGRAM_KEY)) {
        ESP_LOGI(TAG, "No program found in NVS.");
        return;
    }

    String programString = preferences.getString(NVS_PROGRAM_KEY, "");
    if (programString.length() == 0) {
        ESP_LOGW(TAG, "Program key exists in NVS but is empty.");
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
    _program.clear(); // Clear only the vector
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
    if (_program.empty()) {
        // If the program is empty, ensure it's cleared from NVS
        clearProgram();
        return;
    }

    JsonDocument doc = getProgramAsJson();
    String programString;
    serializeJson(doc, programString);

    // Check NVS space before writing
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(NULL, &nvs_stats);
    if (err == ESP_OK) {
        size_t required_space = programString.length() + 32; // Approximation: length + entry overhead
        size_t free_space = nvs_stats.free_entries * 32; // Very rough approximation
        ESP_LOGI(TAG, "Required space: ~%d bytes, Available space: ~%d bytes", required_space, free_space);
        if (required_space > free_space) {
            ESP_LOGE(TAG, "Not enough NVS space to save program!");
            // Optionally, prevent saving, but for now we just log
        }
    }

    if (preferences.putString(NVS_PROGRAM_KEY, programString)) {
        ESP_LOGI(TAG, "Program saved to NVS. Size: %d bytes", programString.length());
    } else {
        ESP_LOGE(TAG, "Failed to save program to NVS. String too large? Size: %d", programString.length());
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

    if (_totalIterations == -1) {
        ESP_LOGI(TAG, "Starting program with infinite iterations.");
    } else {
        ESP_LOGI(TAG, "Starting program with %d iterations.", _totalIterations);
    }
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
    if (preferences.isKey(NVS_PROGRAM_KEY)) {
        preferences.remove(NVS_PROGRAM_KEY);
        ESP_LOGI(TAG, "Program cleared from memory and NVS.");
    } else {
        ESP_LOGI(TAG, "Program cleared from memory.");
    }
}

bool ProgramManager::isRecording() const {
    return _isRecording;
}

void ProgramManager::startRecording() {
    if (_isRunning) {
        ESP_LOGW(TAG, "Cannot start recording, a program is running.");
        return;
    }
    clearProgram(); // Clear any previous program
    _isRecording = true;
    _lastActionRecordTime = millis();
    
    // Initialize the first action with the current state of the vehicle
    _lastRecordedAction.type = ProgramActionType::MOVE_STEER;
    _lastRecordedAction.motorSpeed = 0;
    _lastRecordedAction.steerAngle = 0;
    _lastRecordedAction.duration_ms = 0; // Duration will be calculated on the next step

    ESP_LOGI(TAG, "Recording started.");
}

void ProgramManager::stopRecording() {
    if (!_isRecording) return;
    _isRecording = false;

    // Save the last recorded action
    uint32_t now = millis();
    uint32_t duration = now - _lastActionRecordTime;
    if (duration > 50) { // Debounce: only record steps longer than 50ms
        _lastRecordedAction.duration_ms = duration;
        _program.push_back(_lastRecordedAction);
    }

    ESP_LOGI(TAG, "Recording stopped. Program has %d steps.", _program.size());
    saveProgramToNVS();
}

void ProgramManager::recordStep(int motorSpeed, int steerAngle) {
    if (!_isRecording) return;

    // Check if the action has changed
    if (motorSpeed != _lastRecordedAction.motorSpeed || steerAngle != _lastRecordedAction.steerAngle) {
        uint32_t now = millis();
        uint32_t duration = now - _lastActionRecordTime;

        if (duration > 20) { // Debounce: only record steps longer than 5ms
            // Save the previous action with its calculated duration
            _lastRecordedAction.duration_ms = duration;
            _program.push_back(_lastRecordedAction);
            
            // Start the new action
            _lastRecordedAction.type = ProgramActionType::MOVE_STEER;
            _lastRecordedAction.motorSpeed = motorSpeed;
            _lastRecordedAction.steerAngle = steerAngle;
            _lastActionRecordTime = now;
        }
    }
}

void ProgramManager::loop() {
    if (!_isRunning) {
        return;
    }

    uint32_t now = millis();
    if (now - _actionStartTime >= _program[_currentActionIndex].duration_ms) {
        _currentActionIndex++;
        if (_currentActionIndex >= _program.size()) {
            // End of one iteration
            _currentIteration++;
            
            bool should_continue = false;
            if (_totalIterations == -1) { // Infinite loop
                ESP_LOGI(TAG, "Completed iteration, starting next infinite loop.");
                should_continue = true;
            } else if (_currentIteration < _totalIterations) { // Finite loop
                ESP_LOGI(TAG, "Completed iteration %d of %d.", _currentIteration, _totalIterations);
                should_continue = true;
            }

            if (should_continue) {
                _currentActionIndex = 0;
                _actionStartTime = now;
                executeAction(_program[_currentActionIndex]);
            } else {
                // End of program
                ESP_LOGI(TAG, "Finished all %d iterations.", _totalIterations);
                stopProgram();
            }
        } else {
            // Next action
            _actionStartTime = now;
            executeAction(_program[_currentActionIndex]);
        }
    }
}

bool ProgramManager::isRunning() const {
    return _isRunning;
}

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
            case ProgramActionType::WAIT:
                actionJson["action"] = "wait";
                break;
            case ProgramActionType::LIGHTS_CYCLE:
                actionJson["action"] = "lights_cycle";
                break;
            case ProgramActionType::HAZARDS_TOGGLE:
                actionJson["action"] = "hazards_toggle";
                break;
            case ProgramActionType::RIGHT_TURN_TOGGLE:
                actionJson["action"] = "right_turn_toggle";
                break;
            case ProgramActionType::LEFT_TURN_TOGGLE:
                actionJson["action"] = "left_turn_toggle";
                break;
        }
    }
    return doc;
}

void ProgramManager::executeAction(const ProgrammedAction& action) {
    ESP_LOGI(TAG, "Executing action index %d", _currentActionIndex);
    // Stop previous movement before executing next action
    stopAllActions();

    switch (action.type) {
        case ProgramActionType::MOVE_STEER:
            setMotor(action.motorSpeed, action.motorSpeed >= 0, _state);
            setSteer(action.steerAngle, _state);
            break;
        case ProgramActionType::LIGHTS_CYCLE:
            _state->luces++;
            if (_state->luces > 3) _state->luces = 0;
            break;
        case ProgramActionType::HAZARDS_TOGGLE:
            _state->baliza = !_state->baliza;
            break;
        case ProgramActionType::RIGHT_TURN_TOGGLE:
            _state->giroDerecho = !_state->giroDerecho;
            break;
        case ProgramActionType::LEFT_TURN_TOGGLE:
            _state->giroIzquierdo = !_state->giroIzquierdo;
            break;
        case ProgramActionType::WAIT:
            // Do nothing, just wait
            break;
    }
}

void ProgramManager::stopAllActions() {
    // Reset actuators to a safe state
    setMotor(0, true, _state);
    setSteer(0, _state);
}