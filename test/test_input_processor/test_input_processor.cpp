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
    return UNITY_END();
}
