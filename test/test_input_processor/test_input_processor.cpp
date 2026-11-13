#include <unity.h>
#include "InputProcessor.h"

void setUp() {}
void tearDown() {}

const CalibrationPoint TEST_POS_V[] = {
    CalibrationPoint(0, 0.0f),
    CalibrationPoint(500, 5.0f),
    CalibrationPoint(1000, 10.0f)
};

const CalibrationPoint TEST_NEG_V[] = {
    CalibrationPoint(0, 0.0f),
    CalibrationPoint(400, 5.0f),
    CalibrationPoint(900, 10.0f)
};

const CalibrationPoint TEST_TARGET[] = {
    CalibrationPoint(0, 0.0f),
    CalibrationPoint(4095, 20.0f)
};

InputCalibration calibration() {
    InputCalibration c;
    c.interlockOpen = AdcWindow(0, 100);
    c.selectorNegative = AdcWindow(1200, 1600);
    c.selectorPositive = AdcWindow(2500, 2950);
    c.timer.adcMinimum = 0;
    c.timer.adcMaximum = 4095;
    c.timer.secondsMinimum = 5;
    c.timer.secondsMaximum = 120;
    c.positiveVoltageMonitor.points = TEST_POS_V;
    c.positiveVoltageMonitor.count = 3;
    c.negativeVoltageMonitor.points = TEST_NEG_V;
    c.negativeVoltageMonitor.count = 3;
    c.hvOnActiveLow = true;
    c.useFaultInputs = false;
    c.runSwitchDebounceMs = 50;
    return c;
}

RawInputs baseRaw() {
    RawInputs r;
    r.runSwitchPinHigh = false;
    r.positiveHvOnPinHigh = true;
    r.negativeHvOnPinHigh = true;
    r.positiveFaultPinHigh = true;
    r.negativeFaultPinHigh = true;
    r.polaritySelectorAdc = 2700;
    return r;
}

void test_selector_positive_negative_interlock_and_gap() {
    InputProcessor p(calibration()); Inputs in; RawInputs r = baseRaw();
    p.begin(r, 0, in);
    TEST_ASSERT_TRUE(in.interlockClosed);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Polarity::POSITIVE), static_cast<int>(in.selectedPolarity));

    InputEvents ev;
    r.polaritySelectorAdc = 1400; p.update(r, 1, in, ev);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Polarity::NEGATIVE), static_cast<int>(in.selectedPolarity));

    r.polaritySelectorAdc = 0; p.update(r, 2, in, ev);
    TEST_ASSERT_FALSE(in.interlockClosed);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Polarity::UNKNOWN), static_cast<int>(in.selectedPolarity));

    r.polaritySelectorAdc = 2000; p.update(r, 3, in, ev);
    TEST_ASSERT_TRUE(in.interlockClosed);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Polarity::UNKNOWN), static_cast<int>(in.selectedPolarity));
}

void test_hv_on_active_low() {
    InputProcessor p(calibration()); Inputs in; RawInputs r = baseRaw();
    r.positiveHvOnPinHigh = false; p.begin(r, 0, in);
    TEST_ASSERT_TRUE(in.positiveSupply.hvOn);
    TEST_ASSERT_FALSE(in.negativeSupply.hvOn);
}

void test_fault_inputs_ignored_when_disabled() {
    InputProcessor p(calibration()); Inputs in; RawInputs r = baseRaw();
    r.positiveFaultPinHigh = false; r.negativeFaultPinHigh = false; p.begin(r, 0, in);
    TEST_ASSERT_FALSE(in.positiveSupply.fault);
    TEST_ASSERT_FALSE(in.negativeSupply.fault);
}

void test_fault_inputs_decode_when_enabled() {
    InputCalibration c = calibration(); c.useFaultInputs = true; c.faultActiveLow = true;
    InputProcessor p(c); Inputs in; RawInputs r = baseRaw();
    r.positiveFaultPinHigh = false; r.negativeFaultPinHigh = true; p.begin(r, 0, in);
    TEST_ASSERT_TRUE(in.positiveSupply.fault);
    TEST_ASSERT_FALSE(in.negativeSupply.fault);
}

void test_run_switch_debounce_and_one_cycle_events() {
    InputProcessor p(calibration()); Inputs in; RawInputs r = baseRaw(); InputEvents ev;
    p.begin(r, 0, in);
    r.runSwitchPinHigh = true; p.update(r, 10, in, ev);
    TEST_ASSERT_FALSE(in.runSwitchOn); TEST_ASSERT_FALSE(ev.runSwitchTurnedOn);
    p.update(r, 61, in, ev);
    TEST_ASSERT_TRUE(in.runSwitchOn); TEST_ASSERT_TRUE(ev.runSwitchTurnedOn);
    p.update(r, 62, in, ev);
    TEST_ASSERT_FALSE(ev.runSwitchTurnedOn); TEST_ASSERT_FALSE(ev.runSwitchTurnedOff);
}

void test_timer_mapping_and_clamping() {
    InputProcessor p(calibration()); Inputs in; RawSlowInputs r;
    r.timerPotAdc = 0; p.updateSlow(r, in, Polarity::UNKNOWN); TEST_ASSERT_EQUAL_UINT16(5, in.timerSeconds);
    r.timerPotAdc = 4095; p.updateSlow(r, in, Polarity::UNKNOWN); TEST_ASSERT_EQUAL_UINT16(120, in.timerSeconds);
    r.timerPotAdc = 2048; p.updateSlow(r, in, Polarity::UNKNOWN);
    TEST_ASSERT_TRUE(in.timerSeconds >= 62 && in.timerSeconds <= 63);
}

void test_voltage_lookup_interpolation_and_clamp() {
    InputProcessor p(calibration()); Inputs in; RawSlowInputs r;
    r.positiveVoltageAdc = 750; p.updateSlow(r, in, Polarity::POSITIVE);
    TEST_ASSERT_TRUE(in.measuredVoltageValid);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.5f, in.measuredVoltageKv);
    r.positiveVoltageAdc = 4095; p.updateSlow(r, in, Polarity::POSITIVE);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, in.measuredVoltageKv);
}

void test_measurement_uses_requested_run_polarity_not_selector() {
    InputProcessor p(calibration()); Inputs in; RawSlowInputs r;
    in.selectedPolarity = Polarity::NEGATIVE;
    r.positiveVoltageAdc = 500;
    r.negativeVoltageAdc = 900;
    p.updateSlow(r, in, Polarity::POSITIVE);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, in.measuredVoltageKv);
}

void test_missing_tables_are_marked_invalid() {
    InputProcessor p(calibration()); Inputs in; RawSlowInputs r;
    in.selectedPolarity = Polarity::POSITIVE;
    p.updateSlow(r, in, Polarity::POSITIVE);
    TEST_ASSERT_FALSE(in.measuredCurrentValid);
    TEST_ASSERT_FALSE(in.targetVoltageValid);
    TEST_ASSERT_FALSE(in.targetCurrentValid);
}

void test_target_lookup_when_table_is_added() {
    InputCalibration c = calibration();
    c.positiveVoltageTarget.points = TEST_TARGET;
    c.positiveVoltageTarget.count = 2;
    InputProcessor p(c); Inputs in; RawSlowInputs r;
    in.selectedPolarity = Polarity::POSITIVE;
    r.voltagePotAdc = 2048;
    p.updateSlow(r, in, Polarity::UNKNOWN);
    TEST_ASSERT_TRUE(in.targetVoltageValid);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 10.0f, in.targetVoltageKv);
}


// Representative empirical points from Calibration.xlsx, 13-11-2026.
const CalibrationPoint TEST_POS_ESTIMATE[] = {
    CalibrationPoint(759, 0.7f), CalibrationPoint(2646, 14.1f), CalibrationPoint(2801, 15.9f)
};
const CalibrationPoint TEST_NEG_ESTIMATE[] = {
    CalibrationPoint(595, 0.5f), CalibrationPoint(2706, 21.0f), CalibrationPoint(2807, 22.6f)
};
const CalibrationPoint TEST_CURRENT_TARGET_ESTIMATE[] = {
    CalibrationPoint(1121, 0.7f), CalibrationPoint(4095, 14.1f)
};
InputCalibration estimateCalibration() {
    InputCalibration c = calibration();
    c.positiveCurrentEstimate.points = TEST_POS_ESTIMATE;
    c.positiveCurrentEstimate.count = 3;
    c.negativeCurrentEstimate.points = TEST_NEG_ESTIMATE;
    c.negativeCurrentEstimate.count = 3;
    c.positiveCurrentTargetEstimate.points = TEST_CURRENT_TARGET_ESTIMATE;
    c.positiveCurrentTargetEstimate.count = 2;
    c.positiveVoltageTarget.points = TEST_TARGET;
    c.positiveVoltageTarget.count = 2;
    c.voltagePotSaturationAdc = 4095;
    return c;
}

void test_current_estimate_uses_voltage_pot_not_current_pot() {
    InputProcessor p(estimateCalibration()); Inputs in; RawSlowInputs raw;
    in.selectedPolarity = Polarity::POSITIVE;
    raw.voltagePotAdc = 1121; raw.currentPotAdc = 0;
    p.updateSlow(raw, in, Polarity::UNKNOWN);
    TEST_ASSERT_TRUE(in.targetCurrentValid);
    TEST_ASSERT_TRUE(in.targetCurrentIsEstimated);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.7f, in.targetCurrentUa);
    raw.currentPotAdc = 4095;
    p.updateSlow(raw, in, Polarity::UNKNOWN);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.7f, in.targetCurrentUa);
}

void test_running_estimate_resolves_saturated_pot_positions() {
    InputProcessor p(estimateCalibration()); Inputs in; RawSlowInputs raw;
    in.selectedPolarity = Polarity::NEGATIVE; // measurement must follow active run
    raw.voltagePotAdc = 4095; raw.positiveVoltageAdc = 2646;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_TRUE(in.estimatedCurrentValid);
    TEST_ASSERT_FALSE(in.measuredCurrentValid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.1f, in.estimatedCurrentUa);
    raw.positiveVoltageAdc = 2801;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.9f, in.estimatedCurrentUa);
}

void test_negative_estimate_and_interpolation_use_negative_voltage_monitor() {
    InputProcessor p(estimateCalibration()); Inputs in; RawSlowInputs raw;
    in.selectedPolarity = Polarity::POSITIVE; raw.positiveVoltageAdc = 2801;
    raw.negativeVoltageAdc = 2706;
    p.updateSlow(raw, in, Polarity::NEGATIVE);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 21.0f, in.estimatedCurrentUa);
    raw.negativeVoltageAdc = 2756;
    p.updateSlow(raw, in, Polarity::NEGATIVE);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 21.0f + 50.0f / 101.0f * 1.6f, in.estimatedCurrentUa);
    raw.negativeVoltageAdc = 2807;
    p.updateSlow(raw, in, Polarity::NEGATIVE);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 22.6f, in.estimatedCurrentUa);
}

void test_estimates_outside_measured_range_are_unavailable() {
    InputProcessor p(estimateCalibration()); Inputs in; RawSlowInputs raw;
    in.selectedPolarity = Polarity::POSITIVE; raw.voltagePotAdc = 1120;
    raw.positiveVoltageAdc = 758;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_FALSE(in.targetCurrentValid); TEST_ASSERT_FALSE(in.estimatedCurrentValid);
    raw.positiveVoltageAdc = 759;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_TRUE(in.estimatedCurrentValid);
    raw.positiveVoltageAdc = 2802;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_FALSE(in.estimatedCurrentValid);
    p.updateSlow(raw, in, Polarity::UNKNOWN);
    TEST_ASSERT_FALSE(in.estimatedCurrentValid);
}

void test_saturation_invalidates_idle_targets_but_not_running_estimate() {
    InputProcessor p(estimateCalibration()); Inputs in; RawSlowInputs raw;
    in.selectedPolarity = Polarity::POSITIVE; raw.voltagePotAdc = 4094;
    raw.positiveVoltageAdc = 2646;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_FALSE(in.targetPotSaturated); TEST_ASSERT_TRUE(in.targetCurrentValid);
    raw.voltagePotAdc = 4095;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_TRUE(in.targetPotSaturated);
    TEST_ASSERT_FALSE(in.targetVoltageValid); TEST_ASSERT_FALSE(in.targetCurrentValid);
    TEST_ASSERT_TRUE(in.estimatedCurrentValid);
}

void test_actual_current_measurement_remains_separate_from_estimate() {
    InputCalibration c = estimateCalibration();
    c.positiveCurrentMonitor.points = TEST_POS_V; c.positiveCurrentMonitor.count = 3;
    InputProcessor p(c); Inputs in; RawSlowInputs raw;
    raw.positiveCurrentAdc = 500; raw.positiveVoltageAdc = 2646;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_TRUE(in.measuredCurrentValid); TEST_ASSERT_TRUE(in.estimatedCurrentValid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, in.measuredCurrentUa);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.1f, in.estimatedCurrentUa);
}

void test_strict_voltage_ranges_do_not_invent_unmeasured_zero_points() {
    const CalibrationPoint voltage[] = {CalibrationPoint(69, 2.13f), CalibrationPoint(2804, 29.7f)};
    InputCalibration c = estimateCalibration(); c.requireVoltageCalibrationRange = true;
    c.positiveVoltageTarget.points = voltage; c.positiveVoltageTarget.count = 2;
    c.positiveVoltageMonitor.points = voltage; c.positiveVoltageMonitor.count = 2;
    InputProcessor p(c); Inputs in; RawSlowInputs raw; in.selectedPolarity = Polarity::POSITIVE;
    raw.voltagePotAdc = 0; raw.positiveVoltageAdc = 0;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_FALSE(in.targetVoltageValid); TEST_ASSERT_TRUE(in.targetVoltageOutOfRange);
    TEST_ASSERT_FALSE(in.measuredVoltageValid); TEST_ASSERT_TRUE(in.measuredVoltageOutOfRange);
    raw.voltagePotAdc = 69; raw.positiveVoltageAdc = 69;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_TRUE(in.targetVoltageValid); TEST_ASSERT_TRUE(in.measuredVoltageValid);
    raw.positiveVoltageAdc = 2805;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_FALSE(in.measuredVoltageValid); TEST_ASSERT_TRUE(in.measuredVoltageOutOfRange);
}


void test_user_zero_baseline_produces_zero_voltage_and_estimated_current() {
    const CalibrationPoint voltage[] = {CalibrationPoint(0, 0.0f), CalibrationPoint(69, 2.13f)};
    const CalibrationPoint estimate[] = {CalibrationPoint(0, 0.0f), CalibrationPoint(759, 0.7f)};
    const CalibrationPoint baseline[] = {CalibrationPoint(0, 0.0f)};
    InputCalibration c = estimateCalibration(); c.requireVoltageCalibrationRange = true;
    c.positiveVoltageTarget.points = voltage; c.positiveVoltageTarget.count = 2;
    c.positiveVoltageMonitor.points = voltage; c.positiveVoltageMonitor.count = 2;
    c.positiveCurrentTargetEstimate.points = estimate; c.positiveCurrentTargetEstimate.count = 2;
    c.positiveCurrentEstimate.points = estimate; c.positiveCurrentEstimate.count = 2;
    c.positiveCurrentMonitor.points = baseline; c.positiveCurrentMonitor.count = 1;
    InputProcessor p(c); Inputs in; RawSlowInputs raw; in.selectedPolarity = Polarity::POSITIVE;
    p.updateSlow(raw, in, Polarity::POSITIVE);
    TEST_ASSERT_TRUE(in.targetVoltageValid); TEST_ASSERT_TRUE(in.measuredVoltageValid);
    TEST_ASSERT_TRUE(in.targetCurrentValid); TEST_ASSERT_TRUE(in.estimatedCurrentValid);
    TEST_ASSERT_FALSE(in.measuredCurrentValid); TEST_ASSERT_FALSE(in.targetPotSaturated);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, in.targetVoltageKv);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, in.measuredVoltageKv);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, in.targetCurrentUa);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, in.estimatedCurrentUa);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_selector_positive_negative_interlock_and_gap);
    RUN_TEST(test_hv_on_active_low);
    RUN_TEST(test_fault_inputs_ignored_when_disabled);
    RUN_TEST(test_fault_inputs_decode_when_enabled);
    RUN_TEST(test_run_switch_debounce_and_one_cycle_events);
    RUN_TEST(test_timer_mapping_and_clamping);
    RUN_TEST(test_voltage_lookup_interpolation_and_clamp);
    RUN_TEST(test_measurement_uses_requested_run_polarity_not_selector);
    RUN_TEST(test_missing_tables_are_marked_invalid);
    RUN_TEST(test_target_lookup_when_table_is_added);
    RUN_TEST(test_current_estimate_uses_voltage_pot_not_current_pot);
    RUN_TEST(test_running_estimate_resolves_saturated_pot_positions);
    RUN_TEST(test_negative_estimate_and_interpolation_use_negative_voltage_monitor);
    RUN_TEST(test_estimates_outside_measured_range_are_unavailable);
    RUN_TEST(test_saturation_invalidates_idle_targets_but_not_running_estimate);
    RUN_TEST(test_actual_current_measurement_remains_separate_from_estimate);
    RUN_TEST(test_strict_voltage_ranges_do_not_invent_unmeasured_zero_points);
    RUN_TEST(test_user_zero_baseline_produces_zero_voltage_and_estimated_current);
    return UNITY_END();
}
