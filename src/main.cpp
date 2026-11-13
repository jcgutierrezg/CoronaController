#include <Arduino.h>

#include "Controller.h"
#include "Display.h"
#include "Hardware.h"
#include "InputProcessor.h"

#if (defined(DIAGNOSTIC_MODE) + defined(VOLTAGE_CALIBRATION_MODE) + defined(CURRENT_CALIBRATION_MODE)) > 1
#error "Select only one diagnostic or calibration mode"
#endif

#if defined(VOLTAGE_CALIBRATION_MODE) || defined(CURRENT_CALIBRATION_MODE)
#define CALIBRATION_MODE
#endif

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

// Empirical data: Calibration.xlsx, sheet 13-11-2026. Voltage in kV; current in uA.
// Target curves retain the FIRST 4095 row; the voltage-monitor curve estimates the clipped range.
// Current estimates assume the recorded geometry/load and current-limit pot at maximum.
// Zero baselines requested by the owner; duplicate ADC keys are excluded.

// Source cells B3:D21
const CalibrationPoint POSITIVE_VOLTAGE_TARGET[] = {
    CalibrationPoint(0, 0.0f), // User-specified zero baseline.
    CalibrationPoint(133, 2.13f),
    CalibrationPoint(269, 3.12f),
    CalibrationPoint(451, 4.52f),
    CalibrationPoint(626, 5.78f),
    CalibrationPoint(838, 7.25f),
    CalibrationPoint(1008, 8.45f),
    CalibrationPoint(1258, 10.18f),
    CalibrationPoint(1467, 11.77f),
    CalibrationPoint(1677, 13.34f),
    CalibrationPoint(1858, 14.75f),
    CalibrationPoint(2084, 16.44f),
    CalibrationPoint(2461, 19.31f),
    CalibrationPoint(2791, 21.6f),
    CalibrationPoint(3031, 23.3f),
    CalibrationPoint(3375, 25.2f),
    CalibrationPoint(3789, 27.0f),
    CalibrationPoint(3996, 27.7f),
    CalibrationPoint(4095, 28.7f),
};

// Source cells F3:H21
const CalibrationPoint NEGATIVE_VOLTAGE_TARGET[] = {
    CalibrationPoint(0, 0.0f), // User-specified zero baseline.
    CalibrationPoint(127, 2.1f),
    CalibrationPoint(274, 3.23f),
    CalibrationPoint(448, 4.54f),
    CalibrationPoint(638, 5.9f),
    CalibrationPoint(848, 7.39f),
    CalibrationPoint(999, 8.47f),
    CalibrationPoint(1260, 10.28f),
    CalibrationPoint(1488, 11.97f),
    CalibrationPoint(1661, 13.33f),
    CalibrationPoint(1861, 14.88f),
    CalibrationPoint(2096, 16.67f),
    CalibrationPoint(2457, 19.42f),
    CalibrationPoint(2775, 21.7f),
    CalibrationPoint(3053, 23.6f),
    CalibrationPoint(3326, 25.0f),
    CalibrationPoint(3778, 27.0f),
    CalibrationPoint(3982, 27.6f),
    CalibrationPoint(4095, 28.2f),
};

// Source cells C3:D22
const CalibrationPoint POSITIVE_VOLTAGE_MONITOR[] = {
    CalibrationPoint(0, 0.0f), // User-specified zero baseline.
    CalibrationPoint(69, 2.13f),
    CalibrationPoint(166, 3.12f),
    CalibrationPoint(302, 4.52f),
    CalibrationPoint(423, 5.78f),
    CalibrationPoint(570, 7.25f),
    CalibrationPoint(688, 8.45f),
    CalibrationPoint(856, 10.18f),
    CalibrationPoint(1014, 11.77f),
    CalibrationPoint(1175, 13.34f),
    CalibrationPoint(1315, 14.75f),
    CalibrationPoint(1481, 16.44f),
    CalibrationPoint(1767, 19.31f),
    CalibrationPoint(2001, 21.6f),
    CalibrationPoint(2169, 23.3f),
    CalibrationPoint(2349, 25.2f),
    CalibrationPoint(2524, 27.0f),
    CalibrationPoint(2596, 27.7f),
    CalibrationPoint(2696, 28.7f),
    CalibrationPoint(2804, 29.7f),
};

// Source cells G3:H22
const CalibrationPoint NEGATIVE_VOLTAGE_MONITOR[] = {
    CalibrationPoint(0, 0.0f), // User-specified zero baseline.
    CalibrationPoint(65, 2.1f),
    CalibrationPoint(175, 3.23f),
    CalibrationPoint(300, 4.54f),
    CalibrationPoint(430, 5.9f),
    CalibrationPoint(577, 7.39f),
    CalibrationPoint(681, 8.47f),
    CalibrationPoint(858, 10.28f),
    CalibrationPoint(1027, 11.97f),
    CalibrationPoint(1163, 13.33f),
    CalibrationPoint(1318, 14.88f),
    CalibrationPoint(1493, 16.67f),
    CalibrationPoint(1768, 19.42f),
    CalibrationPoint(1999, 21.7f),
    CalibrationPoint(2185, 23.6f),
    CalibrationPoint(2328, 25.0f),
    CalibrationPoint(2524, 27.0f),
    CalibrationPoint(2586, 27.6f),
    CalibrationPoint(2653, 28.2f),
    CalibrationPoint(2809, 29.7f),
};

// Source cells J3:L14
const CalibrationPoint POSITIVE_CURRENT_TARGET_ESTIMATE[] = {
    CalibrationPoint(0, 0.0f), // User-specified zero baseline.
    CalibrationPoint(1121, 0.7f),
    CalibrationPoint(1413, 1.3f),
    CalibrationPoint(1659, 1.9f),
    CalibrationPoint(1862, 2.7f),
    CalibrationPoint(2229, 4.3f),
    CalibrationPoint(2559, 6.2f),
    CalibrationPoint(2977, 8.7f),
    CalibrationPoint(3184, 9.9f),
    CalibrationPoint(3568, 11.7f),
    CalibrationPoint(3946, 13.3f),
    CalibrationPoint(4095, 14.1f),
};

// Source cells N3:P15
const CalibrationPoint NEGATIVE_CURRENT_TARGET_ESTIMATE[] = {
    CalibrationPoint(0, 0.0f), // User-specified zero baseline.
    CalibrationPoint(877, 0.5f),
    CalibrationPoint(1267, 1.8f),
    CalibrationPoint(1534, 2.9f),
    CalibrationPoint(1859, 4.6f),
    CalibrationPoint(2169, 6.5f),
    CalibrationPoint(2384, 8.1f),
    CalibrationPoint(2692, 10.6f),
    CalibrationPoint(2867, 12.0f),
    CalibrationPoint(3162, 14.4f),
    CalibrationPoint(3420, 16.1f),
    CalibrationPoint(3900, 19.0f),
    CalibrationPoint(4095, 21.0f),
};

// Source cells K3:L15
const CalibrationPoint POSITIVE_CURRENT_ESTIMATE[] = {
    CalibrationPoint(0, 0.0f), // User-specified zero baseline.
    CalibrationPoint(759, 0.7f),
    CalibrationPoint(970, 1.3f),
    CalibrationPoint(1160, 1.9f),
    CalibrationPoint(1316, 2.7f),
    CalibrationPoint(1592, 4.3f),
    CalibrationPoint(1849, 6.2f),
    CalibrationPoint(2131, 8.7f),
    CalibrationPoint(2253, 9.9f),
    CalibrationPoint(2432, 11.7f),
    CalibrationPoint(2576, 13.3f),
    CalibrationPoint(2646, 14.1f),
    CalibrationPoint(2801, 15.9f),
};

// Source cells O3:P16
const CalibrationPoint NEGATIVE_CURRENT_ESTIMATE[] = {
    CalibrationPoint(0, 0.0f), // User-specified zero baseline.
    CalibrationPoint(595, 0.5f),
    CalibrationPoint(867, 1.8f),
    CalibrationPoint(1071, 2.9f),
    CalibrationPoint(1314, 4.6f),
    CalibrationPoint(1548, 6.5f),
    CalibrationPoint(1714, 8.1f),
    CalibrationPoint(1940, 10.6f),
    CalibrationPoint(2063, 12.0f),
    CalibrationPoint(2245, 14.4f),
    CalibrationPoint(2370, 16.1f),
    CalibrationPoint(2555, 19.0f),
    CalibrationPoint(2706, 21.0f),
    CalibrationPoint(2807, 22.6f),
};

// Current monitor calibration: ADC count -> measured current magnitude in uA.
// Use Positive monitor / Negative monitor in current-calibration output, not Target pot.
// TODO: add measured nonzero points to EACH user-specified zero baseline,
// ordered by strictly increasing ADC count. Confirm the zero offset during calibration.
// One placeholder keeps the table unavailable (count < 2), so the LCD
// remains a separate unavailable measurement; the LCD can show a marked estimate.
const CalibrationPoint POSITIVE_CURRENT_MONITOR[] = {
    CalibrationPoint(0, 0.0f), // User zero baseline; add measured points to enable readback.
    // Add further CalibrationPoint(adcCount, measuredCurrentUa) entries here.
};

const CalibrationPoint NEGATIVE_CURRENT_MONITOR[] = {
    CalibrationPoint(0, 0.0f), // User zero baseline; add measured points to enable readback.
    // Add further CalibrationPoint(adcCount, measuredCurrentUa) entries here.
};
// Current-monitor tables remain reserved for actual current-sensor calibration.

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

    calibration.voltagePotSaturationAdc = 4095;
    calibration.estimateSaturatedTargets = true;
    // Small endpoint mismatch between separate full-output calibration sweeps:
    // POS current ends at 2801 vs voltage 2804; NEG 2807 vs 2809.
    // Hold the final current estimate within 8 counts, never beyond voltage calibration.
    calibration.currentEstimateUpperToleranceAdc = 8;
    calibration.requireVoltageCalibrationRange = true;

    calibration.positiveVoltageTarget.points = POSITIVE_VOLTAGE_TARGET;
    calibration.positiveVoltageTarget.count = sizeof(POSITIVE_VOLTAGE_TARGET) / sizeof(POSITIVE_VOLTAGE_TARGET[0]);

    calibration.negativeVoltageTarget.points = NEGATIVE_VOLTAGE_TARGET;
    calibration.negativeVoltageTarget.count = sizeof(NEGATIVE_VOLTAGE_TARGET) / sizeof(NEGATIVE_VOLTAGE_TARGET[0]);

    calibration.positiveVoltageMonitor.points = POSITIVE_VOLTAGE_MONITOR;
    calibration.positiveVoltageMonitor.count = sizeof(POSITIVE_VOLTAGE_MONITOR) / sizeof(POSITIVE_VOLTAGE_MONITOR[0]);

    calibration.negativeVoltageMonitor.points = NEGATIVE_VOLTAGE_MONITOR;
    calibration.negativeVoltageMonitor.count = sizeof(NEGATIVE_VOLTAGE_MONITOR) / sizeof(NEGATIVE_VOLTAGE_MONITOR[0]);

    calibration.positiveCurrentTargetEstimate.points = POSITIVE_CURRENT_TARGET_ESTIMATE;
    calibration.positiveCurrentTargetEstimate.count = sizeof(POSITIVE_CURRENT_TARGET_ESTIMATE) / sizeof(POSITIVE_CURRENT_TARGET_ESTIMATE[0]);

    calibration.negativeCurrentTargetEstimate.points = NEGATIVE_CURRENT_TARGET_ESTIMATE;
    calibration.negativeCurrentTargetEstimate.count = sizeof(NEGATIVE_CURRENT_TARGET_ESTIMATE) / sizeof(NEGATIVE_CURRENT_TARGET_ESTIMATE[0]);

    calibration.positiveCurrentEstimate.points = POSITIVE_CURRENT_ESTIMATE;
    calibration.positiveCurrentEstimate.count = sizeof(POSITIVE_CURRENT_ESTIMATE) / sizeof(POSITIVE_CURRENT_ESTIMATE[0]);

    calibration.negativeCurrentEstimate.points = NEGATIVE_CURRENT_ESTIMATE;
    calibration.negativeCurrentEstimate.count = sizeof(NEGATIVE_CURRENT_ESTIMATE) / sizeof(NEGATIVE_CURRENT_ESTIMATE[0]);

    // A single placeholder remains invalid until at least two measured points exist.
    calibration.positiveCurrentMonitor.points = POSITIVE_CURRENT_MONITOR;
    calibration.positiveCurrentMonitor.count =
        sizeof(POSITIVE_CURRENT_MONITOR) / sizeof(POSITIVE_CURRENT_MONITOR[0]);

    calibration.negativeCurrentMonitor.points = NEGATIVE_CURRENT_MONITOR;
    calibration.negativeCurrentMonitor.count =
        sizeof(NEGATIVE_CURRENT_MONITOR) / sizeof(NEGATIVE_CURRENT_MONITOR[0]);

    // Assembled PCB: OFF = LOW, ON = HIGH (active-high HV_ON).
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

#ifdef CALIBRATION_MODE
// Reuse the normal slow acquisition; logging must not add ADC sampling.
RawSlowInputs calibrationSlowInputs;
constexpr uint32_t CALIBRATION_PRINT_PERIOD_MS = 500;
uint32_t lastCalibrationPrintMs = 0;
char calibrationLine[320];
size_t calibrationLineLength = 0;
size_t calibrationLineSent = 0;

void printCalibration(uint32_t now) {
    // Finish each readable block without waiting for serial buffer space.
    if (calibrationLineSent == calibrationLineLength &&
        now - lastCalibrationPrintMs >= CALIBRATION_PRINT_PERIOD_MS) {
        lastCalibrationPrintMs = now;
#ifdef VOLTAGE_CALIBRATION_MODE
        const int length = snprintf(calibrationLine, sizeof(calibrationLine),
            "\nVOLTAGE CALIBRATION (ADC counts)\n"
            "Target pot:       %u\n"
            "Positive monitor: %u\n"
            "Negative monitor: %u\n",
            static_cast<unsigned>(calibrationSlowInputs.voltagePotAdc),
            static_cast<unsigned>(calibrationSlowInputs.positiveVoltageAdc),
            static_cast<unsigned>(calibrationSlowInputs.negativeVoltageAdc));
#else
        const int length = snprintf(calibrationLine, sizeof(calibrationLine),
            "\nCURRENT CALIBRATION (ADC counts)\n"
            "Target pot:       %u\n"
            "Positive monitor: %u\n"
            "Negative monitor: %u\n",
            static_cast<unsigned>(calibrationSlowInputs.currentPotAdc),
            static_cast<unsigned>(calibrationSlowInputs.positiveCurrentAdc),
            static_cast<unsigned>(calibrationSlowInputs.negativeCurrentAdc));
#endif
        calibrationLineSent = 0;
        calibrationLineLength = length > 0 && static_cast<size_t>(length) < sizeof(calibrationLine)
            ? static_cast<size_t>(length) : 0;
    }

    const int available = Serial.availableForWrite();
    if (available <= 0 || calibrationLineSent == calibrationLineLength) return;
    const size_t remaining = calibrationLineLength - calibrationLineSent;
    const size_t chunk = remaining < static_cast<size_t>(available)
        ? remaining : static_cast<size_t>(available);
    calibrationLineSent += Serial.write(
        reinterpret_cast<const uint8_t*>(calibrationLine + calibrationLineSent), chunk);
}
#endif

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

#ifndef CALIBRATION_MODE
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

#endif

void setup() {
    Serial.begin(115200);

    // First hardware action inside begin(): ENABLE is driven LOW.
    hardware.begin();

    const uint32_t now = millis();

    const RawInputs raw = hardware.readFastInputs();
    inputProcessor.begin(raw, now, inputs);

    const RawSlowInputs slow = hardware.readSlowInputs();
    inputProcessor.updateSlow(slow, inputs, inputs.selectedPolarity);
#ifdef CALIBRATION_MODE
    calibrationSlowInputs = slow;
#endif

    controller.begin(now);
    hardware.applyOutputs(controller.outputs());

    display.begin();
    display.update(controller, inputs, now);

    lastSlowInputMs = now;
    lastDisplayMs = now;
    lastDebugMs = now;
#ifdef CALIBRATION_MODE
    lastCalibrationPrintMs = now;
#endif

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
#ifdef CALIBRATION_MODE
        calibrationSlowInputs = slow;
#endif
    }

    if (now - lastDisplayMs >= DISPLAY_PERIOD_MS) {
        lastDisplayMs = now;
        display.update(controller, inputs, now);
    }

#ifdef CALIBRATION_MODE
    printCalibration(now);
#else
    printDebug(now);
#endif
}

#endif