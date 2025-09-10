#include "ProgramManager.h"
#include "actuators.h" // For setMotor, setSteer
#include <esp_log.h>

static const char* TAG = "ProgramManager";

ProgramManager::ProgramManager(VehicleState* state) : _state(state) {}

void ProgramManager::loadProgram(const JsonArray& programJson) {
    clearProgram();
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
    ESP_LOGI(TAG, "Loaded %d actions into program.", _program.size());
}

void ProgramManager::startProgram() {
    if (_program.empty() || _isRunning) {
        ESP_LOGW(TAG, "Cannot start program. Empty or already running.");
        return;
    }
    _isRunning = true;
    _currentActionIndex = 0;
    _actionStartTime = millis();
    ESP_LOGI(TAG, "Starting program.");
    executeAction(_program[_currentActionIndex]);
}

void ProgramManager::stopProgram() {
    if (!_isRunning) return;
    _isRunning = false;
    stopAllActions();
    ESP_LOGI(TAG, "Program stopped.");
}

void ProgramManager::clearProgram() {
    stopProgram();
    _program.clear();
    ESP_LOGI(TAG, "Program cleared.");
}

void ProgramManager::loop() {
    if (!_isRunning) {
        return;
    }

    uint32_t now = millis();
    if (now - _actionStartTime >= _program[_currentActionIndex].duration_ms) {
        _currentActionIndex++;
        if (_currentActionIndex >= _program.size()) {
            // End of program
            stopProgram();
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