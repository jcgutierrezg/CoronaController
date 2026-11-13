#pragma once

#include <stdint.h>
#include <rgb_lcd.h>
#include "Controller.h"
#include "Types.h"

struct DisplayFrame {
    char top[17];
    char bottom[17];
};

struct RgbColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    RgbColor(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}
};

class Display {
public:
    void begin();
    void update(const Controller& controller, const Inputs& inputs, uint32_t now);

private:
    rgb_lcd lcd_;
    DisplayFrame previousFrame_ = {"", ""};
    State previousColorState_ = State::STARTUP;
    bool firstUpdate_ = true;

    DisplayFrame makeFrame(const Controller& controller, const Inputs& inputs, uint32_t now) const;
    static RgbColor colorForState(State state);
    static char polaritySymbol(Polarity polarity);
    static const char* faultText(FaultCode fault);
    static const char* startCheckText(StartCheck check);
    void writeLineIfChanged(uint8_t row, const char* newLine, char* previousLine);
    void setStateColor(State state);
};
