#pragma once

#include <stdint.h>
#include "Types.h"

class Controller {
public:
    explicit Controller(const ControllerConfig& config);

    void begin(uint32_t now);
    void update(const Inputs& inputs, const InputEvents& events, uint32_t now);

    State state() const;
    StartingPhase startingPhase() const;
    FaultCode faultCode() const;
    StartCheck lastStartCheck() const;
    const RunConfig& currentRun() const;
    const Outputs& outputs() const;
    uint16_t remainingRunSeconds(uint32_t now) const;

private:
    ControllerConfig config_;
    State state_ = State::STARTUP;
    StartingPhase startingPhase_ = StartingPhase::VALIDATING;
    StopReason stopReason_ = StopReason::NONE;
    FaultCode faultCode_ = FaultCode::NONE;
    StartCheck lastStartCheck_ = StartCheck::OK;
    RunConfig currentRun_;
    Outputs outputs_;

    uint32_t stateEnteredAtMs_ = 0;
    uint32_t startingPhaseEnteredAtMs_ = 0;
    uint32_t runStartedAtMs_ = 0;

    bool faultResetInteractionSeen_ = false;
    bool polarityUnknownActive_ = false;
    uint32_t polarityUnknownStartedAtMs_ = 0;

    void transitionTo(State newState, uint32_t now);
    void updateStartup(const Inputs& inputs, uint32_t now);
    void updateIdle(const Inputs& inputs, const InputEvents& events, uint32_t now);
    void updateInterlockOpen(const Inputs& inputs, uint32_t now);
    void updateStarting(const Inputs& inputs, uint32_t now);
    void updateRunning(const Inputs& inputs, uint32_t now);
    void updateStopping(const Inputs& inputs, uint32_t now);
    void updateComplete(const Inputs& inputs, uint32_t now);
    void updateAborted(const Inputs& inputs, uint32_t now);
    void updateFault(const Inputs& inputs, const InputEvents& events, uint32_t now);

    StartCheck checkStartConditions(const Inputs& inputs) const;
    const PowerSupplyStatus* activePowerSupply(const Inputs& inputs) const;
    const PowerSupplyStatus* inactivePowerSupply(const Inputs& inputs) const;
    void captureRunConfiguration(const Inputs& inputs);
    void clearRunConfiguration();
    bool runTimerExpired(uint32_t now) const;
    bool hvIsConfirmedOff(const Inputs& inputs) const;
    bool enabledPolarityIsValid(const Inputs& inputs, uint32_t now);
    void deriveOutputs();
};
