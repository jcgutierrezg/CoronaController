#pragma once
#include <stdint.h>

namespace Pins {
constexpr uint8_t ENABLE = 23;
constexpr uint8_t POLARITY_SELECTOR = 4;

constexpr uint8_t RUN_SWITCH = 32;

// Final intended convention after hardware correction: CCW = ADC 0, CW = ADC max.
constexpr uint8_t VOLTAGE_POT = 13;
constexpr uint8_t CURRENT_POT = 34;
constexpr uint8_t TIMER_POT = 14;

constexpr uint8_t POSITIVE_VOLTAGE_MONITOR = 27;
constexpr uint8_t POSITIVE_CURRENT_MONITOR = 25;
constexpr uint8_t NEGATIVE_VOLTAGE_MONITOR = 26;
constexpr uint8_t NEGATIVE_CURRENT_MONITOR = 33;

constexpr uint8_t POSITIVE_HV_ON = 36;
constexpr uint8_t NEGATIVE_HV_ON = 35;

constexpr uint8_t POSITIVE_FAULT = 39;
constexpr uint8_t NEGATIVE_FAULT = 16;
}
