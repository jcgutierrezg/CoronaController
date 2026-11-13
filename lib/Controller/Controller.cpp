#include "Controller.h"

namespace {
// TODO: validate these against measured HVPS response times.
constexpr uint32_t HV_START_TIMEOUT_MS = 500;
constexpr uint32_t HV_STOP_TIMEOUT_MS = 500;
constexpr uint32_t ABORTED_DISPLAY_TIME_MS = 2500;

// Short UNKNOWN grace to tolerate relay/ADC transients; opposite polarity faults immediately.
// TODO: tune from logged hardware data.
constexpr uint32_t POLARITY_UNKNOWN_GRACE_MS = 100;
}

Controller::Controller(const ControllerConfig& config) : config_(config) {}

void Controller::begin(uint32_t now) {
    state_ = State::STARTUP;
    startingPhase_ = StartingPhase::VALIDATING;
    stopReason_ = StopReason::NONE;
    faultCode_ = FaultCode::NONE;
    lastStartCheck_ = StartCheck::OK;
    currentRun_ = RunConfig{};
    outputs_ = Outputs{};
    stateEnteredAtMs_ = now;
    startingPhaseEnteredAtMs_ = now;
    runStartedAtMs_ = 0;
    faultResetInteractionSeen_ = false;
    polarityUnknownActive_ = false;
    polarityUnknownStartedAtMs_ = 0;
    deriveOutputs();
}

void Controller::update(const Inputs& inputs, const InputEvents& events, uint32_t now) {
    switch (state_) {
        case State::STARTUP: updateStartup(inputs, now); break;
        case State::IDLE: updateIdle(inputs, events, now); break;
        case State::STARTING: updateStarting(inputs, now); break;
        case State::RUNNING: updateRunning(inputs, now); break;
        case State::STOPPING: updateStopping(inputs, now); break;
        case State::COMPLETE: updateComplete(inputs, now); break;
        case State::ABORTED: updateAborted(inputs, now); break;
        case State::INTERLOCK_OPEN: updateInterlockOpen(inputs, now); break;
        case State::FAULT: updateFault(inputs, events, now); break;
    }
    deriveOutputs();
}

void Controller::transitionTo(State newState, uint32_t now) {
    if (newState == state_) return;

    state_ = newState;
    stateEnteredAtMs_ = now;

    switch (newState) {
        case State::STARTING:
            startingPhase_ = StartingPhase::VALIDATING;
            lastStartCheck_ = StartCheck::OK;
            stopReason_ = StopReason::NONE;
            clearRunConfiguration();
            polarityUnknownActive_ = false;
            break;

        case State::RUNNING:
            runStartedAtMs_ = now;
            polarityUnknownActive_ = false;
            break;

        case State::FAULT:
            faultResetInteractionSeen_ = false;
            polarityUnknownActive_ = false;
            break;

        default:
            break;
    }
}

void Controller::updateStartup(const Inputs& inputs, uint32_t now) {
    if (inputs.runSwitchOn) {
        faultCode_ = FaultCode::STARTUP_SWITCH_ON;
        transitionTo(State::FAULT, now);
        return;
    }

    if (inputs.positiveSupply.hvOn || inputs.negativeSupply.hvOn) {
        faultCode_ = FaultCode::HV_PRESENT_BEFORE_START;
        transitionTo(State::FAULT, now);
        return;
    }

    if (!inputs.interlockClosed) {
        transitionTo(State::INTERLOCK_OPEN, now);
        return;
    }

    transitionTo(State::IDLE, now);
}

void Controller::updateIdle(const Inputs& inputs, const InputEvents& events, uint32_t now) {
    if (!inputs.runSwitchOn) lastStartCheck_ = StartCheck::OK;

    if (!inputs.interlockClosed) {
        transitionTo(State::INTERLOCK_OPEN, now);
        return;
    }

    if (events.runSwitchTurnedOn) {
        transitionTo(State::STARTING, now);
    }
}

void Controller::updateInterlockOpen(const Inputs& inputs, uint32_t now) {
    // No auto-resume: door must be closed AND run switch must be OFF.
    if (!inputs.interlockClosed) return;
    if (inputs.runSwitchOn) return;
    transitionTo(State::IDLE, now);
}

void Controller::updateStarting(const Inputs& inputs, uint32_t now) {
    /*
     * Once ENABLE has been commanded ON,
     * physical switch OFF is acted upon immediately.
     */
    if (startingPhase_ == StartingPhase::WAITING_FOR_HV_ON &&
        !inputs.rawRunSwitchOn) {

        stopReason_ = StopReason::ABORTED;
        transitionTo(State::STOPPING, now);

        return;
    }

    /*
     * During validation HV has not been enabled yet,
     * so use the debounced switch state.
     */
    if (startingPhase_ == StartingPhase::VALIDATING &&
         !inputs.runSwitchOn) {

        transitionTo(State::ABORTED, now);

        return;
    }

    if (!inputs.interlockClosed) {
        if (startingPhase_ == StartingPhase::VALIDATING) {
            transitionTo(State::INTERLOCK_OPEN, now);
        } else {
            faultCode_ = FaultCode::INTERLOCK_OPENED_WHILE_ENABLED;
            transitionTo(State::FAULT, now);
        }
        return;
    }

    if (startingPhase_ == StartingPhase::VALIDATING) {
        const StartCheck result = checkStartConditions(inputs);
        lastStartCheck_ = result;

        if (result == StartCheck::OK) {
            captureRunConfiguration(inputs);
            startingPhase_ = StartingPhase::WAITING_FOR_HV_ON;
            startingPhaseEnteredAtMs_ = now;
            polarityUnknownActive_ = false;
            return;
        }

        if (result == StartCheck::POSITIVE_HV_ALREADY_ON ||
            result == StartCheck::NEGATIVE_HV_ALREADY_ON) {
            faultCode_ = FaultCode::HV_PRESENT_BEFORE_START;
            transitionTo(State::FAULT, now);
            return;
        }

        // Rejected start, not machine fault.
        transitionTo(State::IDLE, now);
        return;
    }

    const PowerSupplyStatus* active = activePowerSupply(inputs);
    const PowerSupplyStatus* inactive = inactivePowerSupply(inputs);

    if (active == nullptr || inactive == nullptr) {
        faultCode_ = FaultCode::INVALID_INTERNAL_STATE;
        transitionTo(State::FAULT, now);
        return;
    }

    if (!enabledPolarityIsValid(inputs, now)) return;


    if (inactive->hvOn) {
        faultCode_ = FaultCode::WRONG_SUPPLY_ACTIVE;
        transitionTo(State::FAULT, now);
        return;
    }

    if (config_.useSupplyFaultInputs && active->fault) {
        faultCode_ = (currentRun_.polarity == Polarity::POSITIVE)
            ? FaultCode::POSITIVE_SUPPLY_FAULT
            : FaultCode::NEGATIVE_SUPPLY_FAULT;
        transitionTo(State::FAULT, now);
        return;
    }

    if (active->hvOn) {
        transitionTo(State::RUNNING, now);
        return;
    }

    if (now - startingPhaseEnteredAtMs_ >= HV_START_TIMEOUT_MS) {
        faultCode_ = FaultCode::HV_START_TIMEOUT;
        transitionTo(State::FAULT, now);
    }
}

void Controller::updateRunning(const Inputs& inputs, uint32_t now) {
    const PowerSupplyStatus* active = activePowerSupply(inputs);
    const PowerSupplyStatus* inactive = inactivePowerSupply(inputs);

    if (active == nullptr || inactive == nullptr) {
        faultCode_ = FaultCode::INVALID_INTERNAL_STATE;
        transitionTo(State::FAULT, now);
        return;
    }

    // Safety-invalidating events beat manual abort and timer completion.
    if (!inputs.interlockClosed) {
        faultCode_ = FaultCode::INTERLOCK_OPENED_WHILE_ENABLED;
        transitionTo(State::FAULT, now);
        return;
    }

    if (!enabledPolarityIsValid(inputs, now)) return;


    if (inactive->hvOn) {
        faultCode_ = FaultCode::WRONG_SUPPLY_ACTIVE;
        transitionTo(State::FAULT, now);
        return;
    }

    if (config_.useSupplyFaultInputs && active->fault) {
        faultCode_ = (currentRun_.polarity == Polarity::POSITIVE)
            ? FaultCode::POSITIVE_SUPPLY_FAULT
            : FaultCode::NEGATIVE_SUPPLY_FAULT;
        transitionTo(State::FAULT, now);
        return;
    }

    if (!inputs.rawRunSwitchOn) {
        stopReason_ = StopReason::ABORTED;
        transitionTo(State::STOPPING, now);
        return;
    }

    if (!active->hvOn) {
        faultCode_ = FaultCode::HV_LOST_WHILE_RUNNING;
        transitionTo(State::FAULT, now);
        return;
    }

    if (runTimerExpired(now)) {
        stopReason_ = StopReason::COMPLETED;
        transitionTo(State::STOPPING, now);
    }
}

void Controller::updateStopping(const Inputs& inputs, uint32_t now) {
    const PowerSupplyStatus* active = activePowerSupply(inputs);
    const PowerSupplyStatus* inactive = inactivePowerSupply(inputs);

    if (active == nullptr || inactive == nullptr) {
        faultCode_ = FaultCode::INVALID_INTERNAL_STATE;
        transitionTo(State::FAULT, now);
        return;
    }

    // Until actual HV_OFF is confirmed, energized safety rules still apply.
    if (active->hvOn) {
        if (!inputs.interlockClosed) {
            faultCode_ = FaultCode::INTERLOCK_OPENED_WHILE_ENABLED;
            transitionTo(State::FAULT, now);
            return;
        }

        if (!enabledPolarityIsValid(inputs, now)) return;


        if (inactive->hvOn) {
            faultCode_ = FaultCode::WRONG_SUPPLY_ACTIVE;
            transitionTo(State::FAULT, now);
            return;
        }

        if (config_.useSupplyFaultInputs && active->fault) {
            faultCode_ = (currentRun_.polarity == Polarity::POSITIVE)
                ? FaultCode::POSITIVE_SUPPLY_FAULT
                : FaultCode::NEGATIVE_SUPPLY_FAULT;
            transitionTo(State::FAULT, now);
            return;
        }
    }

    if (!active->hvOn) {
        if (stopReason_ == StopReason::COMPLETED) {
            transitionTo(State::COMPLETE, now);
            return;
        }

        if (stopReason_ == StopReason::ABORTED) {
            transitionTo(State::ABORTED, now);
            return;
        }

        faultCode_ = FaultCode::INVALID_INTERNAL_STATE;
        transitionTo(State::FAULT, now);
        return;
    }

    if (now - stateEnteredAtMs_ >= HV_STOP_TIMEOUT_MS) {
        faultCode_ = FaultCode::HV_STOP_TIMEOUT;
        transitionTo(State::FAULT, now);
    }
}

void Controller::updateComplete(const Inputs& inputs, uint32_t now) {
    if (inputs.runSwitchOn) return;

    if (!inputs.interlockClosed) {
        transitionTo(State::INTERLOCK_OPEN, now);
        return;
    }

    clearRunConfiguration();
    transitionTo(State::IDLE, now);
}

void Controller::updateAborted(const Inputs& inputs, uint32_t now) {
    if (now - stateEnteredAtMs_ < ABORTED_DISPLAY_TIME_MS) return;
    if (inputs.runSwitchOn) return;

    if (!inputs.interlockClosed) {
        clearRunConfiguration();
        transitionTo(State::INTERLOCK_OPEN, now);
        return;
    }

    clearRunConfiguration();
    transitionTo(State::IDLE, now);
}

void Controller::updateFault(const Inputs& inputs, const InputEvents& events, uint32_t now) {
    // Latched fault. A post-fault switch interaction is required.
    if (events.runSwitchTurnedOn || events.runSwitchTurnedOff) {
        faultResetInteractionSeen_ = true;
    }

    if (!faultResetInteractionSeen_) return;
    if (inputs.runSwitchOn) return;
    if (!hvIsConfirmedOff(inputs)) return;

    faultCode_ = FaultCode::NONE;
    clearRunConfiguration();

    if (!inputs.interlockClosed) {
        transitionTo(State::INTERLOCK_OPEN, now);
    } else {
        transitionTo(State::IDLE, now);
    }
}

StartCheck Controller::checkStartConditions(const Inputs& inputs) const {
    if (!inputs.interlockClosed) return StartCheck::INTERLOCK_OPEN;
    if (inputs.selectedPolarity == Polarity::UNKNOWN) return StartCheck::POLARITY_UNKNOWN;

    if (inputs.positiveSupply.hvOn) return StartCheck::POSITIVE_HV_ALREADY_ON;
    if (inputs.negativeSupply.hvOn) return StartCheck::NEGATIVE_HV_ALREADY_ON;
    return StartCheck::OK;
}


const PowerSupplyStatus* Controller::activePowerSupply(const Inputs& inputs) const {
    if (currentRun_.polarity == Polarity::POSITIVE) return &inputs.positiveSupply;
    if (currentRun_.polarity == Polarity::NEGATIVE) return &inputs.negativeSupply;
    return nullptr;
}

const PowerSupplyStatus* Controller::inactivePowerSupply(const Inputs& inputs) const {
    if (currentRun_.polarity == Polarity::POSITIVE) return &inputs.negativeSupply;
    if (currentRun_.polarity == Polarity::NEGATIVE) return &inputs.positiveSupply;
    return nullptr;
}

void Controller::captureRunConfiguration(const Inputs& inputs) {
    currentRun_.polarity = inputs.selectedPolarity;
    currentRun_.durationMs = static_cast<uint32_t>(inputs.timerSeconds) * 1000UL;
}

void Controller::clearRunConfiguration() {
    currentRun_ = RunConfig{};
    stopReason_ = StopReason::NONE;
    runStartedAtMs_ = 0;
    polarityUnknownActive_ = false;
}

bool Controller::runTimerExpired(uint32_t now) const {
    return now - runStartedAtMs_ >= currentRun_.durationMs;
}

bool Controller::hvIsConfirmedOff(const Inputs& inputs) const {
    return !inputs.positiveSupply.hvOn && !inputs.negativeSupply.hvOn;
}

bool Controller::enabledPolarityIsValid(const Inputs& inputs, uint32_t now) {
    if (inputs.selectedPolarity != Polarity::UNKNOWN &&
        inputs.selectedPolarity != currentRun_.polarity) {
        faultCode_ = FaultCode::POLARITY_CHANGED_WHILE_ENABLED;
        transitionTo(State::FAULT, now);
        return false;
    }

    if (inputs.selectedPolarity == currentRun_.polarity) {
        polarityUnknownActive_ = false;
        return true;
    }

    if (inputs.selectedPolarity == Polarity::UNKNOWN) {
        if (!polarityUnknownActive_) {
            polarityUnknownActive_ = true;
            polarityUnknownStartedAtMs_ = now;
            return true;
        }

        if (now - polarityUnknownStartedAtMs_ >= POLARITY_UNKNOWN_GRACE_MS) {
            faultCode_ = FaultCode::POLARITY_SIGNAL_INVALID_WHILE_ENABLED;
            transitionTo(State::FAULT, now);
            return false;
        }

        return true;
    }

    return true;
}

void Controller::deriveOutputs() {
    outputs_.enableHv = false;

    if (state_ == State::STARTING && startingPhase_ == StartingPhase::WAITING_FOR_HV_ON) {
        outputs_.enableHv = true;
        return;
    }

    if (state_ == State::RUNNING) {
        outputs_.enableHv = true;
    }
}

State Controller::state() const { return state_; }
StartingPhase Controller::startingPhase() const { return startingPhase_; }
FaultCode Controller::faultCode() const { return faultCode_; }
StartCheck Controller::lastStartCheck() const { return lastStartCheck_; }
const RunConfig& Controller::currentRun() const { return currentRun_; }
const Outputs& Controller::outputs() const { return outputs_; }

uint16_t Controller::remainingRunSeconds(uint32_t now) const {
    if (state_ != State::RUNNING || currentRun_.durationMs == 0) return 0;

    const uint32_t elapsed = now - runStartedAtMs_;
    if (elapsed >= currentRun_.durationMs) return 0;

    const uint32_t remaining = currentRun_.durationMs - elapsed;
    return static_cast<uint16_t>((remaining + 999UL) / 1000UL);
}
