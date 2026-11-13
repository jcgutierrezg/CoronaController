#pragma once

#include <stdint.h>
#include "RawInputs.h"
#include "RawSlowInputs.h"
#include "Types.h"

struct AdcWindow {
    uint16_t minimum = 0;
    uint16_t maximum = 0;

    AdcWindow() {}
    AdcWindow(uint16_t minValue, uint16_t maxValue)
        : minimum(minValue), maximum(maxValue) {}

    bool contains(uint16_t value) const {
        return value >= minimum && value <= maximum;
    }
};

struct CalibrationPoint {
    uint16_t adc = 0;
    float value = 0.0f;

    CalibrationPoint() {}
    CalibrationPoint(uint16_t adcValue, float physicalValue)
        : adc(adcValue), value(physicalValue) {}
};

struct LookupTable {
    const CalibrationPoint* points = nullptr;
    uint8_t count = 0;
};

struct TimerCalibration {
    uint16_t adcMinimum = 0;
    uint16_t adcMaximum = 4095;
    uint16_t secondsMinimum = 5;
    uint16_t secondsMaximum = 120;
};

struct InputCalibration {
    AdcWindow interlockOpen;
    AdcWindow selectorPositive;
    AdcWindow selectorNegative;

    TimerCalibration timer;

    // TODO: empirical target lookup tables.
    LookupTable positiveVoltageTarget;
    LookupTable negativeVoltageTarget;
    LookupTable positiveCurrentTarget;
    LookupTable negativeCurrentTarget;

    // Estimates with current limit at maximum: voltage-pot ADC -> expected uA,
    // and voltage-monitor ADC -> estimated uA. Never actual current feedback.
    LookupTable positiveCurrentTargetEstimate;
    LookupTable negativeCurrentTargetEstimate;
    LookupTable positiveCurrentEstimate;
    LookupTable negativeCurrentEstimate;
    uint16_t voltagePotSaturationAdc = 0; // zero disables saturation handling
    bool requireVoltageCalibrationRange = false;
    bool estimateSaturatedTargets = false;
    uint16_t currentEstimateUpperToleranceAdc = 0; // endpoint hold; zero keeps strict range

    LookupTable positiveVoltageMonitor;
    LookupTable negativeVoltageMonitor;
    LookupTable positiveCurrentMonitor;
    LookupTable negativeCurrentMonitor;

    bool hvOnActiveLow = true;
    bool useFaultInputs = false;
    bool faultActiveLow = true;
    uint32_t runSwitchDebounceMs = 50;
};

class InputProcessor {
public:
    explicit InputProcessor(const InputCalibration& calibration);

    void begin(const RawInputs& raw, uint32_t now, Inputs& inputs);
    void update(const RawInputs& raw, uint32_t now, Inputs& inputs, InputEvents& events);
    void updateSlow(const RawSlowInputs& raw, Inputs& inputs, Polarity measurementPolarity);

private:
    InputCalibration calibration_;
    bool initialized_ = false;
    bool lastRawRunSwitchOn_ = false;
    bool stableRunSwitchOn_ = false;
    uint32_t lastRunSwitchChangeMs_ = 0;

    void updateRunSwitch(bool rawSwitchOn, uint32_t now, Inputs& inputs, InputEvents& events);
    void updatePolaritySelector(uint16_t adc, Inputs& inputs) const;
    void updatePowerSupplyStatus(const RawInputs& raw, Inputs& inputs) const;
    uint16_t decodeTimerSeconds(uint16_t adc) const;
    float interpolateCalibration(uint16_t adc, const LookupTable& table) const;
    static bool tableAvailable(const LookupTable& table);
    static bool tableCovers(uint16_t adc, const LookupTable& table);
    uint16_t adcForCalibratedValue(float value, const LookupTable& table) const;
    bool currentEstimateCovers(uint16_t adc, const LookupTable& estimate, const LookupTable& voltage) const;
    static bool decodeDigitalSignal(bool pinHigh, bool activeLow);
};
