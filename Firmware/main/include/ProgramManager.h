#ifndef PROGRAM_MANAGER_H
#define PROGRAM_MANAGER_H

#include <vector>
#include <ArduinoJson.h>
#include "state.h" // To get access to VehicleState
#include <Preferences.h>

// Enum for identifying the type of action
enum class ProgramActionType {
    MOVE_STEER,
    LIGHTS_CYCLE,
    HAZARDS_TOGGLE,
    RIGHT_TURN_TOGGLE,
    LEFT_TURN_TOGGLE,
    WAIT // A special action for pausas
};

// Structure for a single action in a program
struct ProgrammedAction {
    ProgramActionType type;
    uint32_t duration_ms; // Duration of the action in milliseconds

    // Parameters for the action (not all will be used in every action)
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

    void loop(); // Main method to be called in the application's loop

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

    void executeAction(const ProgrammedAction& action);
    void stopAllActions();
    void saveProgramToNVS();
    void parseProgramFromJson(const JsonArray& programJson);
};

#endif // PROGRAM_MANAGER_H