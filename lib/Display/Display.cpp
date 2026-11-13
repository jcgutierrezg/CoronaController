#include "Display.h"
#include <stdio.h>
#include <string.h>

namespace {
void padLine(char* line) {
    const size_t length = strlen(line);
    if (length >= 16) {
        line[16] = '\0';
        return;
    }
    for (size_t i = length; i < 16; ++i) line[i] = ' ';
    line[16] = '\0';
}
}

void Display::begin() {
    lcd_.begin(16, 2);
    lcd_.setRGB(255, 255, 255);
    lcd_.setCursor(0, 0);
    lcd_.print("HV CORONA BOX   ");
    lcd_.setCursor(0, 1);
    lcd_.print("STARTUP         ");
    firstUpdate_ = true;
}

void Display::update(const Controller& controller, const Inputs& inputs, uint32_t now) {
    const State state = controller.state();

    if (firstUpdate_ || state != previousColorState_) {
        setStateColor(state);
        previousColorState_ = state;
    }

    DisplayFrame frame = makeFrame(controller, inputs, now);
    writeLineIfChanged(0, frame.top, previousFrame_.top);
    writeLineIfChanged(1, frame.bottom, previousFrame_.bottom);
    firstUpdate_ = false;
}

DisplayFrame Display::makeFrame(const Controller& controller, const Inputs& inputs, uint32_t now) const {
    DisplayFrame frame = {"", ""};

    switch (controller.state()) {
        case State::STARTUP:
            snprintf(frame.top, sizeof(frame.top), "STARTUP");
            snprintf(frame.bottom, sizeof(frame.bottom), "SELF CHECK");
            break;

        case State::IDLE:
            snprintf(frame.top, sizeof(frame.top), "IDLE       %03us", inputs.timerSeconds);
            if (controller.lastStartCheck() != StartCheck::OK && inputs.runSwitchOn) {
                snprintf(frame.bottom, sizeof(frame.bottom), "%s", startCheckText(controller.lastStartCheck()));
            } else if (inputs.targetVoltageValid && inputs.targetCurrentValid) {
                const char sign = polaritySymbol(inputs.selectedPolarity);
                snprintf(frame.bottom, sizeof(frame.bottom), "%c%.1fkV %.1fuA", sign, inputs.targetVoltageKv, inputs.targetCurrentUa);
            } else {
                snprintf(frame.bottom, sizeof(frame.bottom), "%c TARGET CAL TODO", polaritySymbol(inputs.selectedPolarity));
            }
            break;

        case State::STARTING:
            snprintf(frame.top, sizeof(frame.top), "STARTING");
            snprintf(frame.bottom, sizeof(frame.bottom), "%s",
                     controller.startingPhase() == StartingPhase::VALIDATING ? "CHECKING..." : "WAIT FOR HV");
            break;

        case State::RUNNING: {
            snprintf(frame.top, sizeof(frame.top), "RUNNING    %03us", controller.remainingRunSeconds(now));
            const char sign = polaritySymbol(controller.currentRun().polarity);
            if (inputs.measuredVoltageValid && inputs.measuredCurrentValid) {
                snprintf(frame.bottom, sizeof(frame.bottom), "%c%.1fkV %.1fuA", sign, inputs.measuredVoltageKv, inputs.measuredCurrentUa);
            } else if (inputs.measuredVoltageValid) {
                snprintf(frame.bottom, sizeof(frame.bottom), "%c%.1fkV I:TODO", sign, inputs.measuredVoltageKv);
            } else {
                snprintf(frame.bottom, sizeof(frame.bottom), "MONITOR CAL TODO");
            }
            break;
        }

        case State::STOPPING:
            snprintf(frame.top, sizeof(frame.top), "STOPPING");
            snprintf(frame.bottom, sizeof(frame.bottom), "WAIT HV OFF");
            break;

        case State::COMPLETE:
            snprintf(frame.top, sizeof(frame.top), "COMPLETE");
            snprintf(frame.bottom, sizeof(frame.bottom), "SWITCH OFF");
            break;

        case State::ABORTED:
            snprintf(frame.top, sizeof(frame.top), "ABORTED");
            snprintf(frame.bottom, sizeof(frame.bottom), "INCOMPLETE RUN");
            break;

        case State::INTERLOCK_OPEN:
            snprintf(frame.top, sizeof(frame.top), "INTERLOCK OPEN");
            snprintf(frame.bottom, sizeof(frame.bottom), "CLOSE + SW OFF");
            break;

        case State::FAULT:
            snprintf(frame.top, sizeof(frame.top), "FAULT");
            snprintf(frame.bottom, sizeof(frame.bottom), "%s", faultText(controller.faultCode()));
            break;
    }

    padLine(frame.top);
    padLine(frame.bottom);
    return frame;
}

RgbColor Display::colorForState(State state) {
    switch (state) {
        case State::STARTUP: return RgbColor(255, 255, 255);
        case State::IDLE: return RgbColor(0, 255, 255);
        case State::STARTING: return RgbColor(255, 255, 255);
        case State::RUNNING: return RgbColor(255, 0, 255);
        case State::STOPPING: return RgbColor(255, 255, 255);
        case State::COMPLETE: return RgbColor(0, 255, 0);
        case State::ABORTED: return RgbColor(255, 255, 0);
        case State::INTERLOCK_OPEN: return RgbColor(0, 0, 255);
        case State::FAULT: return RgbColor(255, 0, 0);
    }
    return RgbColor(255, 255, 255);
}

char Display::polaritySymbol(Polarity polarity) {
    if (polarity == Polarity::POSITIVE) return '+';
    if (polarity == Polarity::NEGATIVE) return '-';
    return '?';
}

const char* Display::faultText(FaultCode fault) {
    switch (fault) {
        case FaultCode::NONE: return "NONE";
        case FaultCode::STARTUP_SWITCH_ON: return "SWITCH ON BOOT";
        case FaultCode::INVALID_INTERNAL_STATE: return "INTERNAL ERROR";
        case FaultCode::HV_PRESENT_BEFORE_START: return "HV ALREADY ON";
        case FaultCode::HV_START_TIMEOUT: return "HV START TIMEOUT";
        case FaultCode::HV_STOP_TIMEOUT: return "HV STOP TIMEOUT";
        case FaultCode::HV_LOST_WHILE_RUNNING: return "HV LOST";
        case FaultCode::WRONG_SUPPLY_ACTIVE: return "WRONG HVPS ON";
        case FaultCode::INTERLOCK_OPENED_WHILE_ENABLED: return "DOOR OPENED";
        case FaultCode::POLARITY_CHANGED_WHILE_ENABLED: return "POLARITY CHANGE";
        case FaultCode::POLARITY_SIGNAL_INVALID_WHILE_ENABLED: return "POLARITY SIGNAL";
        case FaultCode::POSITIVE_SUPPLY_FAULT: return "POS HVPS FAULT";
        case FaultCode::NEGATIVE_SUPPLY_FAULT: return "NEG HVPS FAULT";
    }
    return "UNKNOWN FAULT";
}

const char* Display::startCheckText(StartCheck check) {
    switch (check) {
        case StartCheck::OK: return "OK";
        case StartCheck::INTERLOCK_OPEN: return "DOOR OPEN";
        case StartCheck::POLARITY_UNKNOWN: return "POLARITY ?";
        case StartCheck::POSITIVE_HV_ALREADY_ON: return "POS HV ON";
        case StartCheck::NEGATIVE_HV_ALREADY_ON: return "NEG HV ON";
    }
    return "START BLOCKED";
}

void Display::writeLineIfChanged(uint8_t row, const char* newLine, char* previousLine) {
    if (!firstUpdate_ && strcmp(newLine, previousLine) == 0) return;
    lcd_.setCursor(0, row);
    lcd_.print(newLine);
    strncpy(previousLine, newLine, 16);
    previousLine[16] = '\0';
}

void Display::setStateColor(State state) {
    const RgbColor color = colorForState(state);
    lcd_.setRGB(color.red, color.green, color.blue);
}
