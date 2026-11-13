#pragma once
#include <stdint.h>

struct RawSlowInputs {
    uint16_t timerPotAdc = 0;
    uint16_t voltagePotAdc = 0;
    uint16_t currentPotAdc = 0;
    uint16_t positiveVoltageAdc = 0;
    uint16_t positiveCurrentAdc = 0;
    uint16_t negativeVoltageAdc = 0;
    uint16_t negativeCurrentAdc = 0;
};
