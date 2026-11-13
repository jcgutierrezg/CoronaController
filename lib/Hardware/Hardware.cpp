#include <Arduino.h>
#include "Hardware.h"
#include "Pins.h"

namespace {
// Required on current breadboard to reject relay-induced selector ADC transients.
// TODO: reduce after grounding/decoupling/PCB improvements if measurements allow.
constexpr uint16_t FAST_ADC_SAMPLES = 250;
constexpr uint16_t SLOW_ADC_SAMPLES = 100;
}

void Hardware::begin() {
    pinMode(Pins::ENABLE, OUTPUT);
    digitalWrite(Pins::ENABLE, LOW); // safe state first

    pinMode(Pins::RUN_SWITCH, INPUT);
    pinMode(Pins::POSITIVE_HV_ON, INPUT);
    pinMode(Pins::NEGATIVE_HV_ON, INPUT);
    pinMode(Pins::POSITIVE_FAULT, INPUT);
    pinMode(Pins::NEGATIVE_FAULT, INPUT);

    pinMode(Pins::POLARITY_SELECTOR, INPUT);
    pinMode(Pins::VOLTAGE_POT, INPUT);
    pinMode(Pins::CURRENT_POT, INPUT);
    pinMode(Pins::TIMER_POT, INPUT);

    pinMode(Pins::POSITIVE_VOLTAGE_MONITOR, INPUT);
    pinMode(Pins::POSITIVE_CURRENT_MONITOR, INPUT);
    pinMode(Pins::NEGATIVE_VOLTAGE_MONITOR, INPUT);
    pinMode(Pins::NEGATIVE_CURRENT_MONITOR, INPUT);

    analogReadResolution(12);
}

RawInputs Hardware::readFastInputs() const {
    RawInputs raw;

    raw.runSwitchPinHigh = digitalRead(Pins::RUN_SWITCH) == HIGH;
    raw.polaritySelectorAdc = readAdcFast(Pins::POLARITY_SELECTOR);

    raw.positiveHvOnPinHigh = digitalRead(Pins::POSITIVE_HV_ON) == HIGH;
    raw.negativeHvOnPinHigh = digitalRead(Pins::NEGATIVE_HV_ON) == HIGH;
    raw.positiveFaultPinHigh = digitalRead(Pins::POSITIVE_FAULT) == HIGH;
    raw.negativeFaultPinHigh = digitalRead(Pins::NEGATIVE_FAULT) == HIGH;

    return raw;
}

RawSlowInputs Hardware::readSlowInputs() const {
    RawSlowInputs raw;
    raw.timerPotAdc = readAdcSlow(Pins::TIMER_POT);
    raw.voltagePotAdc = readAdcSlow(Pins::VOLTAGE_POT);
    raw.currentPotAdc = readAdcSlow(Pins::CURRENT_POT);
    raw.positiveVoltageAdc = readAdcSlow(Pins::POSITIVE_VOLTAGE_MONITOR);
    raw.positiveCurrentAdc = readAdcSlow(Pins::POSITIVE_CURRENT_MONITOR);
    raw.negativeVoltageAdc = readAdcSlow(Pins::NEGATIVE_VOLTAGE_MONITOR);
    raw.negativeCurrentAdc = readAdcSlow(Pins::NEGATIVE_CURRENT_MONITOR);
    return raw;
}

void Hardware::applyOutputs(const Outputs& outputs) {
    digitalWrite(Pins::ENABLE, outputs.enableHv ? HIGH : LOW);
}

uint16_t Hardware::readAdcFast(uint8_t pin) {
    uint32_t total = 0;
    for (uint16_t i = 0; i < FAST_ADC_SAMPLES; ++i) total += analogRead(pin);
    return static_cast<uint16_t>(total / FAST_ADC_SAMPLES);
}

uint16_t Hardware::readAdcSlow(uint8_t pin) {
    uint32_t total = 0;
    for (uint16_t i = 0; i < SLOW_ADC_SAMPLES; ++i) total += analogRead(pin);
    return static_cast<uint16_t>(total / SLOW_ADC_SAMPLES);
}
