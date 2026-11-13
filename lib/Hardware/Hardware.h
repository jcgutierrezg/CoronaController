#pragma once

#include <stdint.h>
#include "RawInputs.h"
#include "RawSlowInputs.h"
#include "Types.h"

class Hardware {
public:
    void begin();
    RawInputs readFastInputs() const;
    RawSlowInputs readSlowInputs() const;
    void applyOutputs(const Outputs& outputs);

private:
    static uint16_t readAdcFast(uint8_t pin);
    static uint16_t readAdcSlow(uint8_t pin);
};
