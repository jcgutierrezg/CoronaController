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
    inputs.targetCurrentIsEstimated = false;
    inputs.targetVoltageIsEstimated = false;
    inputs.targetVoltageOutOfRange = false;
    inputs.measuredVoltageOutOfRange = false;
    inputs.targetPotSaturated = calibration_.voltagePotSaturationAdc != 0 &&
        raw.voltagePotAdc >= calibration_.voltagePotSaturationAdc;
    inputs.measuredVoltageValid = false;
    inputs.measuredCurrentValid = false;
    inputs.estimatedCurrentValid = false;

    if (inputs.selectedPolarity != Polarity::UNKNOWN) {
        const bool positive = inputs.selectedPolarity == Polarity::POSITIVE;
        const LookupTable& voltage = positive ? calibration_.positiveVoltageTarget : calibration_.negativeVoltageTarget;
        const LookupTable& current = positive ? calibration_.positiveCurrentTarget : calibration_.negativeCurrentTarget;
        const LookupTable& estimate = positive ? calibration_.positiveCurrentTargetEstimate : calibration_.negativeCurrentTargetEstimate;
        inputs.targetVoltageOutOfRange = calibration_.requireVoltageCalibrationRange &&
            tableAvailable(voltage) && !tableCovers(raw.voltagePotAdc, voltage);
        inputs.targetVoltageValid = tableAvailable(voltage) && !inputs.targetPotSaturated && !inputs.targetVoltageOutOfRange;
        if (inputs.targetVoltageValid)
            inputs.targetVoltageKv = interpolateCalibration(raw.voltagePotAdc, voltage);
        if (tableAvailable(estimate)) {
            inputs.targetCurrentIsEstimated = true;
            inputs.targetCurrentValid = !inputs.targetPotSaturated && tableCovers(raw.voltagePotAdc, estimate);
            if (inputs.targetCurrentValid)
                inputs.targetCurrentUa = interpolateCalibration(raw.voltagePotAdc, estimate);
        } else {
            inputs.targetCurrentValid = tableAvailable(current);
            if (inputs.targetCurrentValid)
                inputs.targetCurrentUa = interpolateCalibration(raw.currentPotAdc, current);
        }

        if (inputs.targetPotSaturated && calibration_.estimateSaturatedTargets) {
            // The pot cannot resolve this region. Use a calibrated midpoint while
            // HV is off, or a valid live voltage monitor in the clipped region.
            const LookupTable& monitor = positive ? calibration_.positiveVoltageMonitor : calibration_.negativeVoltageMonitor;
            const LookupTable& currentEstimate = positive ? calibration_.positiveCurrentEstimate : calibration_.negativeCurrentEstimate;
            const uint16_t monitorAdc = positive ? raw.positiveVoltageAdc : raw.negativeVoltageAdc;
            const bool hvOn = positive ? inputs.positiveSupply.hvOn : inputs.negativeSupply.hvOn;
            inputs.targetVoltageValid = false;
            inputs.targetCurrentValid = false;
            if (tableAvailable(voltage) && tableAvailable(monitor)) {
                const float lowerKv = voltage.points[voltage.count - 1].value;
                const float upperKv = monitor.points[monitor.count - 1].value;
                if (upperKv >= lowerKv && lowerKv >= monitor.points[0].value) {
                    uint16_t estimateAdc = adcForCalibratedValue((lowerKv + upperKv) * 0.5f, monitor);
                    if (hvOn && tableCovers(monitorAdc, monitor) &&
                        interpolateCalibration(monitorAdc, monitor) >= lowerKv) {
                        estimateAdc = monitorAdc;
                    }
                    inputs.targetVoltageKv = interpolateCalibration(estimateAdc, monitor);
                    inputs.targetVoltageValid = true;
                    inputs.targetVoltageIsEstimated = true;
                    inputs.targetVoltageOutOfRange = false;
                    inputs.targetCurrentIsEstimated = true;
                    inputs.targetCurrentValid = currentEstimateCovers(estimateAdc, currentEstimate, monitor);
                    if (inputs.targetCurrentValid)
                        inputs.targetCurrentUa = interpolateCalibration(estimateAdc, currentEstimate);
                }
            }
        }
    }

    if (measurementPolarity != Polarity::UNKNOWN) {
        const bool positive = measurementPolarity == Polarity::POSITIVE;
        const uint16_t voltageAdc = positive ? raw.positiveVoltageAdc : raw.negativeVoltageAdc;
        const uint16_t currentAdc = positive ? raw.positiveCurrentAdc : raw.negativeCurrentAdc;
        const LookupTable& voltage = positive ? calibration_.positiveVoltageMonitor : calibration_.negativeVoltageMonitor;
        const LookupTable& current = positive ? calibration_.positiveCurrentMonitor : calibration_.negativeCurrentMonitor;
        const LookupTable& estimate = positive ? calibration_.positiveCurrentEstimate : calibration_.negativeCurrentEstimate;
        inputs.measuredVoltageOutOfRange = calibration_.requireVoltageCalibrationRange &&
            tableAvailable(voltage) && !tableCovers(voltageAdc, voltage);
        inputs.measuredVoltageValid = tableAvailable(voltage) && !inputs.measuredVoltageOutOfRange;
        inputs.measuredCurrentValid = tableAvailable(current);
        if (inputs.measuredVoltageValid)
            inputs.measuredVoltageKv = interpolateCalibration(voltageAdc, voltage);
        if (inputs.measuredCurrentValid)
            inputs.measuredCurrentUa = interpolateCalibration(currentAdc, current);
        inputs.estimatedCurrentValid = currentEstimateCovers(voltageAdc, estimate, voltage);
        if (inputs.estimatedCurrentValid)
            inputs.estimatedCurrentUa = interpolateCalibration(voltageAdc, estimate);
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

uint16_t InputProcessor::adcForCalibratedValue(float value, const LookupTable& table) const {
    // Inverse interpolation for the monotonic voltage-monitor calibration.
    if (value <= table.points[0].value) return table.points[0].adc;
    for (uint8_t i = 0; i < table.count - 1; ++i) {
        const CalibrationPoint& lower = table.points[i];
        const CalibrationPoint& upper = table.points[i + 1];
        if (value >= lower.value && value <= upper.value && upper.value > lower.value) {
            const float fraction = (value - lower.value) / (upper.value - lower.value);
            return static_cast<uint16_t>(lower.adc + fraction * (upper.adc - lower.adc) + 0.5f);
        }
    }
    return table.points[table.count - 1].adc;
}

bool InputProcessor::currentEstimateCovers(uint16_t adc, const LookupTable& estimate, const LookupTable& voltage) const {
    if (tableCovers(adc, estimate)) return true;
    if (!tableAvailable(estimate) || !tableCovers(adc, voltage)) return false;
    const uint16_t lastAdc = estimate.points[estimate.count - 1].adc;
    // interpolateCalibration clamps to the final current point here: no extrapolation.
    return adc > lastAdc &&
        static_cast<uint32_t>(adc - lastAdc) <= calibration_.currentEstimateUpperToleranceAdc;
}

bool InputProcessor::tableCovers(uint16_t adc, const LookupTable& table) {
    return tableAvailable(table) && adc >= table.points[0].adc &&
        adc <= table.points[table.count - 1].adc;
}

bool InputProcessor::tableAvailable(const LookupTable& table) {
    return table.points != nullptr && table.count >= 2;
}

bool InputProcessor::decodeDigitalSignal(bool pinHigh, bool activeLow) {
    return activeLow ? !pinHigh : pinHigh;
}
