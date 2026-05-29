#ifndef PROGRAM_MANAGER_H
#define PROGRAM_MANAGER_H

#include <vector>
#include <string>
#include <ArduinoJson.h>
#include "state.h"

enum class ProgramActionType {
    MOVE_STEER,
    LIGHTS_CYCLE,
    HAZARDS_TOGGLE,
    RIGHT_TURN_TOGGLE,
    LEFT_TURN_TOGGLE,
    WAIT
};

struct ProgrammedAction {
    ProgramActionType type;
    uint32_t duration_ms;
    int motorSpeed;
    int steerAngle;
};

class ProgramManager {
public:
    ProgramManager(VehicleState* state);

    void loadProgram(const JsonArray& programJson);
    void loadProgramFromNVS();
    void startProgram(int iterations = 1);
    void stopProgram();
    void clearProgram();

    void startRecording();
    void stopRecording();
    void recordStep(int motorSpeed, int steerAngle);
    bool isRecording() const;

    void loop();

    bool isRunning() const;
    JsonDocument getProgramAsJson();

private:
    VehicleState* _state;
    std::vector<ProgrammedAction> _program;
    bool _isRunning = false;
    size_t _currentActionIndex = 0;
    uint32_t _actionStartTime = 0;
    int _totalIterations = 0;
    int _currentIteration = 0;

    bool _isRecording = false;
    uint32_t _lastActionRecordTime = 0;
    ProgrammedAction _lastRecordedAction;

    void executeAction(const ProgrammedAction& action);
    void stopAllActions();
    void saveProgramToNVS();
    void parseProgramFromJson(const JsonArray& programJson);
};

#endif // PROGRAM_MANAGER_H
