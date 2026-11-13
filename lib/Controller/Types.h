#pragma once

#include <stdint.h>

enum class State {
    STARTUP,
    IDLE,
    STARTING,
    RUNNING,
    STOPPING,
    COMPLETE,
    ABORTED,
    INTERLOCK_OPEN,
    FAULT
};

enum class StartingPhase {
    VALIDATING,
    WAITING_FOR_HV_ON
};

enum class Polarity {
    POSITIVE,
    NEGATIVE,
    UNKNOWN
};

enum class StopReason {
    NONE,
    COMPLETED,
    ABORTED
};

enum class StartCheck {
    OK,
    INTERLOCK_OPEN,
    POLARITY_UNKNOWN,
    POSITIVE_HV_ALREADY_ON,
    NEGATIVE_HV_ALREADY_ON
};

enum class FaultCode {
    NONE,
    STARTUP_SWITCH_ON,
    INVALID_INTERNAL_STATE,
    HV_PRESENT_BEFORE_START,
    HV_START_TIMEOUT,
    HV_STOP_TIMEOUT,
    HV_LOST_WHILE_RUNNING,
    WRONG_SUPPLY_ACTIVE,
    INTERLOCK_OPENED_WHILE_ENABLED,
    POLARITY_CHANGED_WHILE_ENABLED,
    POLARITY_SIGNAL_INVALID_WHILE_ENABLED,
    POSITIVE_SUPPLY_FAULT,
    NEGATIVE_SUPPLY_FAULT
};

struct PowerSupplyStatus {
    bool hvOn = false;
    bool fault = false;
};

struct Inputs {
    bool rawRunSwitchOn = false;
    bool runSwitchOn = false;
    bool interlockClosed = false;
    Polarity selectedPolarity = Polarity::UNKNOWN;
    uint16_t timerSeconds = 0;

    float targetVoltageKv = 0.0f;
    float targetCurrentUa = 0.0f;
    float measuredVoltageKv = 0.0f;
    float measuredCurrentUa = 0.0f;

    bool targetVoltageValid = false;
    bool targetCurrentValid = false;
    bool measuredVoltageValid = false;
    bool measuredCurrentValid = false;

    PowerSupplyStatus positiveSupply;
    PowerSupplyStatus negativeSupply;
};

struct InputEvents {
    bool runSwitchTurnedOn = false;
    bool runSwitchTurnedOff = false;
};

struct RunConfig {
    Polarity polarity = Polarity::UNKNOWN;
    uint32_t durationMs = 0;
};

struct Outputs {
    bool enableHv = false;
};

struct ControllerConfig {
    // TODO: enable after both HVPS FAULT signals are characterized/wired.
    bool useSupplyFaultInputs = false;
};
