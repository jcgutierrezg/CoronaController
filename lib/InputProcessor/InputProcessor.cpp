#include "InputProcessor.h"

InputProcessor::InputProcessor(const InputCalibration& calibration)
    : calibration_(calibration) {}

void InputProcessor::begin(const RawInputs& raw, uint32_t now, Inputs& inputs) {
    lastRawRunSwitchOn_ = raw.runSwitchPinHigh;
    stableRunSwitchOn_ = raw.runSwitchPinHigh;
    lastRunSwitchChangeMs_ = now;
    inputs.rawRunSwitchOn = raw.runSwitchPinHigh;
    inputs.runSwitchOn = stableRunSwitchOn_;

    updatePolaritySelector(raw.polaritySelectorAdc, inputs);
    updatePowerSupplyStatus(raw, inputs);
    initialized_ = true;
}

void InputProcessor::update(const RawInputs& raw, uint32_t now, Inputs& inputs, InputEvents& events) {
    events = InputEvents{};

    if (!initialized_) {
        begin(raw, now, inputs);
        return;
    }

    inputs.rawRunSwitchOn = raw.runSwitchPinHigh;
    
    updateRunSwitch(raw.runSwitchPinHigh, now, inputs, events);
    updatePolaritySelector(raw.polaritySelectorAdc, inputs);
    updatePowerSupplyStatus(raw, inputs);
}

void InputProcessor::updateSlow(const RawSlowInputs& raw, Inputs& inputs, Polarity measurementPolarity) {
    inputs.timerSeconds = decodeTimerSeconds(raw.timerPotAdc);

    inputs.targetVoltageValid = false;
    inputs.targetCurrentValid = false;

    switch (inputs.selectedPolarity) {
        case Polarity::POSITIVE:
            inputs.targetVoltageValid = tableAvailable(calibration_.positiveVoltageTarget);
            inputs.targetCurrentValid = tableAvailable(calibration_.positiveCurrentTarget);
            if (inputs.targetVoltageValid)
                inputs.targetVoltageKv = interpolateCalibration(raw.voltagePotAdc, calibration_.positiveVoltageTarget);
            if (inputs.targetCurrentValid)
                inputs.targetCurrentUa = interpolateCalibration(raw.currentPotAdc, calibration_.positiveCurrentTarget);
            break;

        case Polarity::NEGATIVE:
            inputs.targetVoltageValid = tableAvailable(calibration_.negativeVoltageTarget);
            inputs.targetCurrentValid = tableAvailable(calibration_.negativeCurrentTarget);
            if (inputs.targetVoltageValid)
                inputs.targetVoltageKv = interpolateCalibration(raw.voltagePotAdc, calibration_.negativeVoltageTarget);
            if (inputs.targetCurrentValid)
                inputs.targetCurrentUa = interpolateCalibration(raw.currentPotAdc, calibration_.negativeCurrentTarget);
            break;

        case Polarity::UNKNOWN:
        default:
            break;
    }

    inputs.measuredVoltageValid = false;
    inputs.measuredCurrentValid = false;

    switch (measurementPolarity) {
        case Polarity::POSITIVE:
            inputs.measuredVoltageValid = tableAvailable(calibration_.positiveVoltageMonitor);
            inputs.measuredCurrentValid = tableAvailable(calibration_.positiveCurrentMonitor);
            if (inputs.measuredVoltageValid)
                inputs.measuredVoltageKv = interpolateCalibration(raw.positiveVoltageAdc, calibration_.positiveVoltageMonitor);
            if (inputs.measuredCurrentValid)
                inputs.measuredCurrentUa = interpolateCalibration(raw.positiveCurrentAdc, calibration_.positiveCurrentMonitor);
            break;

        case Polarity::NEGATIVE:
            inputs.measuredVoltageValid = tableAvailable(calibration_.negativeVoltageMonitor);
            inputs.measuredCurrentValid = tableAvailable(calibration_.negativeCurrentMonitor);
            if (inputs.measuredVoltageValid)
                inputs.measuredVoltageKv = interpolateCalibration(raw.negativeVoltageAdc, calibration_.negativeVoltageMonitor);
            if (inputs.measuredCurrentValid)
                inputs.measuredCurrentUa = interpolateCalibration(raw.negativeCurrentAdc, calibration_.negativeCurrentMonitor);
            break;

        case Polarity::UNKNOWN:
        default:
            break;
    }
}

void InputProcessor::updateRunSwitch(bool rawSwitchOn, uint32_t now, Inputs& inputs, InputEvents& events) {
    if (rawSwitchOn != lastRawRunSwitchOn_) {
        lastRawRunSwitchOn_ = rawSwitchOn;
        lastRunSwitchChangeMs_ = now;
    }

    if (now - lastRunSwitchChangeMs_ < calibration_.runSwitchDebounceMs) return;
    if (rawSwitchOn == stableRunSwitchOn_) return;

    const bool previous = stableRunSwitchOn_;
    stableRunSwitchOn_ = rawSwitchOn;
    inputs.runSwitchOn = stableRunSwitchOn_;

    if (!previous && stableRunSwitchOn_) events.runSwitchTurnedOn = true;
    if (previous && !stableRunSwitchOn_) events.runSwitchTurnedOff = true;
}

void InputProcessor::updatePolaritySelector(uint16_t adc, Inputs& inputs) const {
    if (calibration_.interlockOpen.contains(adc)) {
        inputs.interlockClosed = false;
        inputs.selectedPolarity = Polarity::UNKNOWN;
        return;
    }

    if (calibration_.selectorPositive.contains(adc)) {
        inputs.interlockClosed = true;
        inputs.selectedPolarity = Polarity::POSITIVE;
        return;
    }

    if (calibration_.selectorNegative.contains(adc)) {
        inputs.interlockClosed = true;
        inputs.selectedPolarity = Polarity::NEGATIVE;
        return;
    }

    inputs.interlockClosed = true;
    inputs.selectedPolarity = Polarity::UNKNOWN;
}

void InputProcessor::updatePowerSupplyStatus(const RawInputs& raw, Inputs& inputs) const {
    inputs.positiveSupply.hvOn = decodeDigitalSignal(raw.positiveHvOnPinHigh, calibration_.hvOnActiveLow);
    inputs.negativeSupply.hvOn = decodeDigitalSignal(raw.negativeHvOnPinHigh, calibration_.hvOnActiveLow);

    if (calibration_.useFaultInputs) {
        inputs.positiveSupply.fault = decodeDigitalSignal(raw.positiveFaultPinHigh, calibration_.faultActiveLow);
        inputs.negativeSupply.fault = decodeDigitalSignal(raw.negativeFaultPinHigh, calibration_.faultActiveLow);
    } else {
        inputs.positiveSupply.fault = false;
        inputs.negativeSupply.fault = false;
    }
}

uint16_t InputProcessor::decodeTimerSeconds(uint16_t adc) const {
    const TimerCalibration& timer = calibration_.timer;
    if (timer.adcMaximum <= timer.adcMinimum) return timer.secondsMinimum;
    if (adc <= timer.adcMinimum) return timer.secondsMinimum;
    if (adc >= timer.adcMaximum) return timer.secondsMaximum;

    const uint32_t adcRange = static_cast<uint32_t>(timer.adcMaximum - timer.adcMinimum);
    const uint32_t secondsRange = static_cast<uint32_t>(timer.secondsMaximum - timer.secondsMinimum);
    const uint32_t offset = static_cast<uint32_t>(adc - timer.adcMinimum);

    return static_cast<uint16_t>(timer.secondsMinimum + (offset * secondsRange) / adcRange);
}

float InputProcessor::interpolateCalibration(uint16_t adc, const LookupTable& table) const {
    if (!tableAvailable(table)) return 0.0f;
    if (adc <= table.points[0].adc) return table.points[0].value;
    if (adc >= table.points[table.count - 1].adc) return table.points[table.count - 1].value;

    for (uint8_t i = 0; i < table.count - 1; ++i) {
        const CalibrationPoint& lower = table.points[i];
        const CalibrationPoint& upper = table.points[i + 1];
        if (adc >= lower.adc && adc <= upper.adc) {
            const float fraction = static_cast<float>(adc - lower.adc) /
                                   static_cast<float>(upper.adc - lower.adc);
            return lower.value + fraction * (upper.value - lower.value);
        }
    }

    return table.points[table.count - 1].value;
}

bool InputProcessor::tableAvailable(const LookupTable& table) {
    return table.points != nullptr && table.count >= 2;
}

bool InputProcessor::decodeDigitalSignal(bool pinHigh, bool activeLow) {
    return activeLow ? !pinHigh : pinHigh;
}
