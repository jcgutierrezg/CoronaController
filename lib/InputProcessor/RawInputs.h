#pragma once
#include <stdint.h>

struct RawInputs {
    bool runSwitchPinHigh = false;
    bool positiveHvOnPinHigh = true;
    bool negativeHvOnPinHigh = true;
    bool positiveFaultPinHigh = true;
    bool negativeFaultPinHigh = true;
    uint16_t polaritySelectorAdc = 0;
};
