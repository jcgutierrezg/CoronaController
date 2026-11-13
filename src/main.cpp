#include <Arduino.h>

#include "Controller.h"
#include "Display.h"
#include "Hardware.h"
#include "InputProcessor.h"

#ifdef DIAGNOSTIC_MODE

Hardware hardware;

constexpr uint32_t PRINT_INTERVAL_MS = 250;

uint32_t lastPrintAtMs = 0;

void printRawInputs(const RawInputs& raw)
{
    Serial.println();
    Serial.println("=== RAW INPUTS ===");

    Serial.print("Run switch pin: ");
    Serial.println(
        raw.runSwitchPinHigh ? "HIGH" : "LOW"
    );

    Serial.print("Polarity selector ADC: ");
    Serial.println(
        raw.polaritySelectorAdc
    );

    Serial.print("POS HV_ON pin: ");
    Serial.println(
        raw.positiveHvOnPinHigh ? "HIGH" : "LOW"
    );

    Serial.print("POS FAULT pin: ");
    Serial.println(
        raw.positiveFaultPinHigh ? "HIGH" : "LOW"
    );

    Serial.print("NEG HV_ON pin: ");
    Serial.println(
        raw.negativeHvOnPinHigh ? "HIGH" : "LOW"
    );

    Serial.print("NEG FAULT pin: ");
    Serial.println(
        raw.negativeFaultPinHigh ? "HIGH" : "LOW"
    );

    Serial.println("==================");
}

void setup()
{
    Serial.begin(115200);

    hardware.begin();

    /*
     * Diagnostic mode must never energize HV.
     */

    Outputs safeOutputs;
    safeOutputs.enableHv = false;

    hardware.applyOutputs(
        safeOutputs
    );

    Serial.println();
    Serial.println("HV Corona Box Diagnostic Mode");
    Serial.println("ENABLE is forced LOW");
    Serial.println("Raw inputs will print every 250 ms");
}

void loop()
{
    const uint32_t now =
        millis();

    /*
     * Explicitly keep ENABLE LOW on every loop.
     */

    Outputs safeOutputs;
    safeOutputs.enableHv = false;

    hardware.applyOutputs(
        safeOutputs
    );

    if (
        now - lastPrintAtMs
        < PRINT_INTERVAL_MS
    ) {
        return;
    }

    lastPrintAtMs = now;

    const RawInputs raw =
        hardware.readFastInputs();

    const RawSlowInputs slow =
        hardware.readSlowInputs();

    printRawInputs(raw);

    Serial.println("--- SLOW INPUTS ---");

    Serial.print("Timer pot ADC: ");
    Serial.println(
        slow.timerPotAdc
    );

    Serial.print("Voltage pot ADC: ");
    Serial.println(
        slow.voltagePotAdc
    );

    Serial.print("Current pot ADC: ");
    Serial.println(
        slow.currentPotAdc
    );

    Serial.print("POS voltage ADC: ");
    Serial.println(
        slow.positiveVoltageAdc
    );

    Serial.print("POS current ADC: ");
    Serial.println(
        slow.positiveCurrentAdc
    );

    Serial.print("NEG voltage ADC: ");
    Serial.println(
        slow.negativeVoltageAdc
    );

    Serial.print("NEG current ADC: ");
    Serial.println(
        slow.negativeCurrentAdc
    );
}

#else


namespace {
constexpr uint32_t SLOW_INPUT_PERIOD_MS = 100;
constexpr uint32_t DISPLAY_PERIOD_MS = 100;
constexpr uint32_t DEBUG_PERIOD_MS = 100;
}

// Measured voltage calibration from this machine.
// TODO: extend beyond 15.3 kV; interpolation intentionally clamps above final point.

const CalibrationPoint POSITIVE_VOLTAGE_TARGET[] = {
    CalibrationPoint(0,   0.0f),
    // CalibrationPoint(64,   0.250f),
    // CalibrationPoint(137,  0.504f),
    // CalibrationPoint(356,  0.962f),
    // CalibrationPoint(590,  1.25f),
    // CalibrationPoint(1000, 1.7f),
    // CalibrationPoint(2080, 2.84f),
    // CalibrationPoint(3218, 5.72f),
    // CalibrationPoint(3757, 10.8f),
    // CalibrationPoint(3933, 16.4f),
    // CalibrationPoint(3999, 19.9f),
    // CalibrationPoint(4072, 25.7f),
    CalibrationPoint(4095, 29.8f)
};


const CalibrationPoint POSITIVE_CURRENT_TARGET[] = {
    CalibrationPoint(0,   0.0f),
    // CalibrationPoint(17,  0.1f),
    // CalibrationPoint(27,  0.9f),
    // CalibrationPoint(40,  2.2f),
    // CalibrationPoint(54,  4.0f),
    // CalibrationPoint(67,  6.1f),
    // CalibrationPoint(82,  9.3f),
    // CalibrationPoint(98, 11.7f),
    //CalibrationPoint(140, 15.5f),
    CalibrationPoint(4095, 999.9f)
};


const CalibrationPoint NEGATIVE_VOLTAGE_TARGET[] = {
    CalibrationPoint(0,   0.0f),
    // CalibrationPoint(46,   0.312f),
    // CalibrationPoint(144,  0.677f),
    // CalibrationPoint(271,  0.984f),
    // CalibrationPoint(305,  1.061f),
    // CalibrationPoint(986,  1.84f),
    // CalibrationPoint(1536, 2.39f),
    // CalibrationPoint(2450, 3.66f),
    // CalibrationPoint(3296, 6.33f),
    // CalibrationPoint(3590, 8.61f),
    // CalibrationPoint(3761, 11.3f),
    // CalibrationPoint(3916, 15.57f),
    // CalibrationPoint(3997, 20.1f),
    // CalibrationPoint(4059, 24.6f),
    CalibrationPoint(4095, 30.0f)
};


const CalibrationPoint NEGATIVE_CURRENT_TARGET[] = {
    CalibrationPoint(0,   0.0f),
    // CalibrationPoint(15,  0.5f),
    // CalibrationPoint(17,  0.1f),
    // CalibrationPoint(24,  1.5f),
    // CalibrationPoint(44,  4.0f),
    // CalibrationPoint(57,  6.1f),
    // CalibrationPoint(71,  8.6f),
    // CalibrationPoint(82, 11.9f),
    // CalibrationPoint(93, 13.2f),
    // CalibrationPoint(122, 18.6f),
    // CalibrationPoint(148, 22.1f),
    CalibrationPoint(4095, 999.9f)
};

const CalibrationPoint POSITIVE_VOLTAGE_MONITOR[] = {
    CalibrationPoint(0, 0.0f),
    CalibrationPoint(195, 5.0f),
    CalibrationPoint(643, 10.0f),
    CalibrationPoint(1137, 15.3f)
};

const CalibrationPoint NEGATIVE_VOLTAGE_MONITOR[] = {
    CalibrationPoint(0, 0.0f),
    CalibrationPoint(203, 5.0f),
    CalibrationPoint(663, 10.0f),
    CalibrationPoint(1189, 15.3f)
};

// TODO: add POSITIVE_CURRENT_MONITOR / NEGATIVE_CURRENT_MONITOR empirical tables.
// TODO: add four empirical target-pot tables (POS/NEG voltage and current).
// Target tables describe expected real corona output, not nominal 0-30 kV / 0-1 mA command range.

ControllerConfig makeControllerConfig() {
    ControllerConfig config;
    config.useSupplyFaultInputs = false;    // TODO: true after FAULT semantics and wiring are verified.
    return config;
}

InputCalibration makeInputCalibration() {
    InputCalibration calibration;

    calibration.interlockOpen = AdcWindow(0, 200);
    calibration.selectorPositive = AdcWindow(800, 1600);
    calibration.selectorNegative = AdcWindow(1900, 2650);

    // Final intended pot wiring convention: CCW = 0, CW = max.
    calibration.timer.adcMinimum = 0;
    calibration.timer.adcMaximum = 4095;
    calibration.timer.secondsMinimum = 5;
    calibration.timer.secondsMaximum = 120;

    // TODO: assign empirical target tables here when measured.
    calibration.positiveVoltageTarget.points =
        POSITIVE_VOLTAGE_TARGET;

    calibration.positiveVoltageTarget.count =
        sizeof(POSITIVE_VOLTAGE_TARGET) /
        sizeof(POSITIVE_VOLTAGE_TARGET[0]);

    calibration.negativeVoltageTarget.points =
        NEGATIVE_VOLTAGE_TARGET;

    calibration.negativeVoltageTarget.count =
        sizeof(NEGATIVE_VOLTAGE_TARGET) /
        sizeof(NEGATIVE_VOLTAGE_TARGET[0]);


    calibration.positiveCurrentTarget.points =
        POSITIVE_CURRENT_TARGET;

    calibration.positiveCurrentTarget.count =
        sizeof(POSITIVE_CURRENT_TARGET) /
        sizeof(POSITIVE_CURRENT_TARGET[0]);


    calibration.negativeCurrentTarget.points =
        NEGATIVE_CURRENT_TARGET;

    calibration.negativeCurrentTarget.count =
        sizeof(NEGATIVE_CURRENT_TARGET) /
        sizeof(NEGATIVE_CURRENT_TARGET[0]);

    calibration.positiveVoltageMonitor.points = POSITIVE_VOLTAGE_MONITOR;
    calibration.positiveVoltageMonitor.count =
        sizeof(POSITIVE_VOLTAGE_MONITOR) / sizeof(POSITIVE_VOLTAGE_MONITOR[0]);

    calibration.negativeVoltageMonitor.points = NEGATIVE_VOLTAGE_MONITOR;
    calibration.negativeVoltageMonitor.count =
        sizeof(NEGATIVE_VOLTAGE_MONITOR) / sizeof(NEGATIVE_VOLTAGE_MONITOR[0]);

    // TODO: assign current-monitor tables after calibration.
    calibration.positiveCurrentMonitor = LookupTable{};
    calibration.negativeCurrentMonitor = LookupTable{};

    // Confirmed on both supplies: OFF = HIGH, ON = LOW.
    calibration.hvOnActiveLow = false;

    // TODO: characterize FAULT. Current observation alone is not enough to define semantics safely.
    calibration.useFaultInputs = false;
    calibration.faultActiveLow = false;

    calibration.runSwitchDebounceMs = 50;
    return calibration;
}

ControllerConfig controllerConfig = makeControllerConfig();
InputCalibration inputCalibration = makeInputCalibration();
Hardware hardware;
InputProcessor inputProcessor(inputCalibration);
Controller controller(controllerConfig);
Display display;
Inputs inputs;
InputEvents events;

uint32_t lastSlowInputMs = 0;
uint32_t lastDisplayMs = 0;
uint32_t lastDebugMs = 0;

Polarity measurementPolarity() {
    switch (controller.state()) {
        case State::STARTING:
        case State::RUNNING:
        case State::STOPPING:
        case State::COMPLETE:
        case State::ABORTED:
        case State::FAULT:
            if (controller.currentRun().polarity != Polarity::UNKNOWN)
                return controller.currentRun().polarity;
            break;
        default:
            break;
    }
    return inputs.selectedPolarity;
}

void printDebug(uint32_t now) {
    if (now - lastDebugMs < DEBUG_PERIOD_MS) return;
    lastDebugMs = now;

    Serial.print("state="); Serial.print(static_cast<int>(controller.state()));
    Serial.print(" switch="); Serial.print(inputs.runSwitchOn);
    Serial.print(" interlock="); Serial.print(inputs.interlockClosed);
    Serial.print(" polarity="); Serial.print(static_cast<int>(inputs.selectedPolarity));
    Serial.print(" timer="); Serial.print(inputs.timerSeconds);
    Serial.print(" enable="); Serial.print(controller.outputs().enableHv);
    Serial.print(" posHV="); Serial.print(inputs.positiveSupply.hvOn);
    Serial.print(" negHV="); Serial.print(inputs.negativeSupply.hvOn);
    Serial.print(" V="); Serial.print(inputs.measuredVoltageKv, 2);
    Serial.print(" fault="); Serial.println(static_cast<int>(controller.faultCode()));
}

void setup() {
    Serial.begin(115200);

    // First hardware action inside begin(): ENABLE is driven LOW.
    hardware.begin();

    const uint32_t now = millis();

    const RawInputs raw = hardware.readFastInputs();
    inputProcessor.begin(raw, now, inputs);

    const RawSlowInputs slow = hardware.readSlowInputs();
    inputProcessor.updateSlow(slow, inputs, inputs.selectedPolarity);

    controller.begin(now);
    hardware.applyOutputs(controller.outputs());

    display.begin();
    display.update(controller, inputs, now);

    lastSlowInputMs = now;
    lastDisplayMs = now;
    lastDebugMs = now;

    // TODO - FINAL HARDENING:
    // Add/configure ESP32 task watchdog after functional behavior is stable.
    // It is supplementary protection only and must not replace the physical interlock,
    // physical enable chain, HV_ON feedback, or state-machine safety logic.
}

void loop() {
    const uint32_t now = millis();

    // Fast path: safety/config input -> controller -> relay output.
    const RawInputs raw = hardware.readFastInputs();
    inputProcessor.update(raw, now, inputs, events);
    controller.update(inputs, events, now);
    hardware.applyOutputs(controller.outputs());

    // Slow path: timer, target estimate, measured V/I.
    if (now - lastSlowInputMs >= SLOW_INPUT_PERIOD_MS) {
        lastSlowInputMs = now;
        const RawSlowInputs slow = hardware.readSlowInputs();
        inputProcessor.updateSlow(slow, inputs, measurementPolarity());
    }

    if (now - lastDisplayMs >= DISPLAY_PERIOD_MS) {
        lastDisplayMs = now;
        display.update(controller, inputs, now);
    }

    printDebug(now);
}

#endif