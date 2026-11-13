#include <unity.h>
#include "Controller.h"

void setUp() {}
void tearDown() {}

ControllerConfig fullConfig() {
    ControllerConfig config;
    config.useSupplyFaultInputs = false;
    return config;
}

ControllerConfig faultEnabledConfig() {
    ControllerConfig config;
    config.useSupplyFaultInputs = true;
    return config;
}

Inputs validPositiveInputs() {
    Inputs inputs;
    inputs.rawRunSwitchOn = false;
    inputs.runSwitchOn = false;
    inputs.interlockClosed = true;
    inputs.selectedPolarity = Polarity::POSITIVE;
    inputs.timerSeconds = 30;
    inputs.positiveSupply.hvOn = false;
    inputs.negativeSupply.hvOn = false;
    inputs.positiveSupply.fault = false;
    inputs.negativeSupply.fault = false;
    return inputs;
}

void bootToIdle(Controller& c, Inputs& in, uint32_t& now) {
    c.begin(now);
    now += 1;
    c.update(in, InputEvents{}, now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::IDLE), static_cast<int>(c.state()));
}

void requestStart(Controller& c, Inputs& in, uint32_t& now) {
    in.rawRunSwitchOn = true;
    in.runSwitchOn = true;
    InputEvents ev;
    ev.runSwitchTurnedOn = true;
    now += 1;
    c.update(in, ev, now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::STARTING), static_cast<int>(c.state()));
}

void validateStart(Controller& c, Inputs& in, uint32_t& now) {
    now += 1;
    c.update(in, InputEvents{}, now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(StartingPhase::WAITING_FOR_HV_ON), static_cast<int>(c.startingPhase()));
    TEST_ASSERT_TRUE(c.outputs().enableHv);
}

void enterRunning(Controller& c, Inputs& in, uint32_t& now) {
    bootToIdle(c, in, now);
    requestStart(c, in, now);
    validateStart(c, in, now);
    in.positiveSupply.hvOn = true;
    now += 1;
    c.update(in, InputEvents{}, now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::RUNNING), static_cast<int>(c.state()));
    TEST_ASSERT_TRUE(c.outputs().enableHv);
}

void test_safe_startup_to_idle() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    bootToIdle(c, in, now);
    TEST_ASSERT_FALSE(c.outputs().enableHv);
}

void test_switch_on_at_boot_faults() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs();
    in.rawRunSwitchOn = true;
    in.runSwitchOn = true; c.begin(0); c.update(in, InputEvents{}, 1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::FAULT), static_cast<int>(c.state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::STARTUP_SWITCH_ON), static_cast<int>(c.faultCode()));
}

void test_hv_present_at_boot_faults() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs();
    in.positiveSupply.hvOn = true; c.begin(0); c.update(in, InputEvents{}, 1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::HV_PRESENT_BEFORE_START), static_cast<int>(c.faultCode()));
}

void test_idle_door_open_enters_interlock_open() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    bootToIdle(c, in, now); in.interlockClosed = false; in.selectedPolarity = Polarity::UNKNOWN;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::INTERLOCK_OPEN), static_cast<int>(c.state()));
}

void test_interlock_open_requires_closed_and_switch_off() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs();
    in.interlockClosed = false; in.selectedPolarity = Polarity::UNKNOWN;
    c.begin(0); c.update(in, InputEvents{}, 1);
    in.interlockClosed = true; in.selectedPolarity = Polarity::POSITIVE; in.rawRunSwitchOn = true; in.runSwitchOn = true;
    c.update(in, InputEvents{}, 2);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::INTERLOCK_OPEN), static_cast<int>(c.state()));
    in.rawRunSwitchOn = false; in.runSwitchOn = false; c.update(in, InputEvents{}, 3);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::IDLE), static_cast<int>(c.state()));
}

void test_start_captures_timer_and_polarity() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    bootToIdle(c, in, now); requestStart(c, in, now); validateStart(c, in, now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Polarity::POSITIVE), static_cast<int>(c.currentRun().polarity));
    TEST_ASSERT_EQUAL_UINT32(30000, c.currentRun().durationMs);
    in.timerSeconds = 120;
    TEST_ASSERT_EQUAL_UINT32(30000, c.currentRun().durationMs);
}


void test_preexisting_hv_at_start_faults() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    bootToIdle(c, in, now); requestStart(c, in, now); in.positiveSupply.hvOn = true;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::HV_PRESENT_BEFORE_START), static_cast<int>(c.faultCode()));
}

void test_start_timeout_faults() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    bootToIdle(c, in, now); requestStart(c, in, now); validateStart(c, in, now);
    now += 501; c.update(in, InputEvents{}, now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::HV_START_TIMEOUT), static_cast<int>(c.faultCode()));
    TEST_ASSERT_FALSE(c.outputs().enableHv);
}

void test_manual_cancel_before_enable_is_aborted() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    bootToIdle(c, in, now); requestStart(c, in, now);
    in.rawRunSwitchOn = false; in.runSwitchOn = false; c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::ABORTED), static_cast<int>(c.state()));
}

void test_manual_stop_running_goes_stopping_then_aborted() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.rawRunSwitchOn = false; in.runSwitchOn = false;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::STOPPING), static_cast<int>(c.state()));
    TEST_ASSERT_FALSE(c.outputs().enableHv);
    in.positiveSupply.hvOn = false; c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::ABORTED), static_cast<int>(c.state()));
}

void test_timer_starts_at_hv_confirmation_and_completes() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    in.timerSeconds = 5;
    bootToIdle(c, in, now); requestStart(c, in, now); validateStart(c, in, now);
    now += 300; // waiting for HV should not consume run timer
    in.positiveSupply.hvOn = true; c.update(in, InputEvents{}, now);
    const uint32_t runStart = now;
    c.update(in, InputEvents{}, runStart + 4999);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::RUNNING), static_cast<int>(c.state()));
    c.update(in, InputEvents{}, runStart + 5000);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::STOPPING), static_cast<int>(c.state()));
    in.positiveSupply.hvOn = false; c.update(in, InputEvents{}, runStart + 5001);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::COMPLETE), static_cast<int>(c.state()));
}

void test_complete_latches_until_switch_off() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    in.timerSeconds = 1;
    enterRunning(c, in, now); now += 1000; c.update(in, InputEvents{}, now);
    in.positiveSupply.hvOn = false; c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::COMPLETE), static_cast<int>(c.state()));
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::COMPLETE), static_cast<int>(c.state()));
    in.rawRunSwitchOn = false; in.runSwitchOn = false; c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::IDLE), static_cast<int>(c.state()));
}

void test_aborted_minimum_display_and_switch_off() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.rawRunSwitchOn = false; in.runSwitchOn = false; c.update(in, InputEvents{}, ++now);
    in.positiveSupply.hvOn = false; c.update(in, InputEvents{}, ++now);
    const uint32_t abortedAt = now;
    c.update(in, InputEvents{}, abortedAt + 2499);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::ABORTED), static_cast<int>(c.state()));
    c.update(in, InputEvents{}, abortedAt + 2500);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::IDLE), static_cast<int>(c.state()));
}

void test_interlock_open_running_faults_and_never_resumes() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now);
    in.interlockClosed = false; in.selectedPolarity = Polarity::UNKNOWN;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::INTERLOCK_OPENED_WHILE_ENABLED), static_cast<int>(c.faultCode()));
    TEST_ASSERT_FALSE(c.outputs().enableHv);
    in.interlockClosed = true; in.selectedPolarity = Polarity::POSITIVE;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::FAULT), static_cast<int>(c.state()));
}

void test_opposite_polarity_running_faults_immediately() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.selectedPolarity = Polarity::NEGATIVE;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::POLARITY_CHANGED_WHILE_ENABLED), static_cast<int>(c.faultCode()));
}

void test_short_unknown_polarity_is_tolerated() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.selectedPolarity = Polarity::UNKNOWN;
    c.update(in, InputEvents{}, ++now);
    now += 50; c.update(in, InputEvents{}, now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::RUNNING), static_cast<int>(c.state()));
    in.selectedPolarity = Polarity::POSITIVE; c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::RUNNING), static_cast<int>(c.state()));
}

void test_sustained_unknown_polarity_faults() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.selectedPolarity = Polarity::UNKNOWN;
    c.update(in, InputEvents{}, ++now);
    now += 101; c.update(in, InputEvents{}, now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::POLARITY_SIGNAL_INVALID_WHILE_ENABLED), static_cast<int>(c.faultCode()));
}

void test_hv_lost_running_faults() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.positiveSupply.hvOn = false;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::HV_LOST_WHILE_RUNNING), static_cast<int>(c.faultCode()));
}

void test_wrong_supply_active_faults() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.negativeSupply.hvOn = true;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::WRONG_SUPPLY_ACTIVE), static_cast<int>(c.faultCode()));
}

void test_supply_fault_when_enabled_faults() {
    Controller c(faultEnabledConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.positiveSupply.fault = true;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::POSITIVE_SUPPLY_FAULT), static_cast<int>(c.faultCode()));
}

void test_safety_fault_beats_manual_abort() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now);
    in.rawRunSwitchOn = false;in.runSwitchOn = false; in.interlockClosed = false; in.selectedPolarity = Polarity::UNKNOWN;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::FAULT), static_cast<int>(c.state()));
}

void test_safety_fault_beats_timer_completion() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    in.timerSeconds = 1; enterRunning(c, in, now);
    in.interlockClosed = false; in.selectedPolarity = Polarity::UNKNOWN;
    now += 1000; c.update(in, InputEvents{}, now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::FAULT), static_cast<int>(c.state()));
}

void test_stopping_waits_for_hv_off_and_times_out() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.rawRunSwitchOn = false; in.runSwitchOn = false;
    c.update(in, InputEvents{}, ++now);
    const uint32_t stoppingAt = now;
    c.update(in, InputEvents{}, stoppingAt + 499);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::STOPPING), static_cast<int>(c.state()));
    c.update(in, InputEvents{}, stoppingAt + 500);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::HV_STOP_TIMEOUT), static_cast<int>(c.faultCode()));
}

void test_stopping_still_faults_on_interlock_while_hv_present() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.rawRunSwitchOn = false; in.runSwitchOn = false; c.update(in, InputEvents{}, ++now);
    in.interlockClosed = false; in.selectedPolarity = Polarity::UNKNOWN;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::INTERLOCK_OPENED_WHILE_ENABLED), static_cast<int>(c.faultCode()));
}

void test_fault_entered_with_switch_on_clears_after_off_event_and_hv_off() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.interlockClosed = false; in.selectedPolarity = Polarity::UNKNOWN;
    c.update(in, InputEvents{}, ++now);
    in.positiveSupply.hvOn = false; in.interlockClosed = true; in.selectedPolarity = Polarity::POSITIVE;
    in.rawRunSwitchOn = false; in.runSwitchOn = false; InputEvents off; off.runSwitchTurnedOff = true;
    c.update(in, off, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::IDLE), static_cast<int>(c.state()));
}

void test_fault_entered_with_switch_off_requires_off_on_off_interaction() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now);
    in.rawRunSwitchOn = false; in.runSwitchOn = false; c.update(in, InputEvents{}, ++now); // STOPPING
    now += 500; c.update(in, InputEvents{}, now); // HV stop timeout; switch already OFF
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::FAULT), static_cast<int>(c.state()));
    in.positiveSupply.hvOn = false;
    c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::FAULT), static_cast<int>(c.state()));
    in.rawRunSwitchOn = true; in.runSwitchOn = true; InputEvents on; on.runSwitchTurnedOn = true; c.update(in, on, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::FAULT), static_cast<int>(c.state()));
    in.rawRunSwitchOn = false; in.runSwitchOn = false; InputEvents off; off.runSwitchTurnedOff = true; c.update(in, off, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::IDLE), static_cast<int>(c.state()));
}

void test_fault_reset_with_interlock_open_goes_interlock_open() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    enterRunning(c, in, now); in.interlockClosed = false; in.selectedPolarity = Polarity::UNKNOWN;
    c.update(in, InputEvents{}, ++now);
    in.positiveSupply.hvOn = false;in.rawRunSwitchOn = false; in.runSwitchOn = false; InputEvents off; off.runSwitchTurnedOff = true;
    c.update(in, off, ++now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::INTERLOCK_OPEN), static_cast<int>(c.state()));
}

void test_output_whitelist_never_enables_in_safe_states() {
    Controller c(fullConfig()); Inputs in = validPositiveInputs(); uint32_t now = 0;
    bootToIdle(c, in, now); TEST_ASSERT_FALSE(c.outputs().enableHv);
    in.interlockClosed = false; in.selectedPolarity = Polarity::UNKNOWN; c.update(in, InputEvents{}, ++now);
    TEST_ASSERT_FALSE(c.outputs().enableHv);
}

void test_operator_abort_with_immediate_hv_loss_is_aborted()
{
    Controller c(fullConfig());
    Inputs in = validPositiveInputs();
    uint32_t now = 0;
    enterRunning(c,in,now);
    in.rawRunSwitchOn = false;
    in.runSwitchOn = false;
    in.positiveSupply.hvOn = false;
    now += 1;
    c.update(in,InputEvents{},now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::STOPPING),static_cast<int>(c.state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::NONE),static_cast<int>(c.faultCode()));
    now += 1;
    c.update(in,InputEvents{},now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::ABORTED),static_cast<int>(c.state()));
}

void test_raw_switch_off_prevents_false_hv_loss_fault()
{
    Controller c(fullConfig());
    Inputs in = validPositiveInputs();
    uint32_t now = 0;
    enterRunning(c,in,now);
    in.rawRunSwitchOn = false;
    in.runSwitchOn = true;
    in.positiveSupply.hvOn = false;
    now += 1;
    c.update(in,InputEvents{},now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::STOPPING),static_cast<int>(c.state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(FaultCode::NONE),static_cast<int>(c.faultCode()));
    now += 1;
    c.update(in,InputEvents{},now);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(State::ABORTED),static_cast<int>(c.state()));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_safe_startup_to_idle);
    RUN_TEST(test_switch_on_at_boot_faults);
    RUN_TEST(test_hv_present_at_boot_faults);
    RUN_TEST(test_idle_door_open_enters_interlock_open);
    RUN_TEST(test_interlock_open_requires_closed_and_switch_off);
    RUN_TEST(test_start_captures_timer_and_polarity);
    RUN_TEST(test_preexisting_hv_at_start_faults);
    RUN_TEST(test_start_timeout_faults);
    RUN_TEST(test_manual_cancel_before_enable_is_aborted);
    RUN_TEST(test_manual_stop_running_goes_stopping_then_aborted);
    RUN_TEST(test_timer_starts_at_hv_confirmation_and_completes);
    RUN_TEST(test_complete_latches_until_switch_off);
    RUN_TEST(test_aborted_minimum_display_and_switch_off);
    RUN_TEST(test_interlock_open_running_faults_and_never_resumes);
    RUN_TEST(test_opposite_polarity_running_faults_immediately);
    RUN_TEST(test_short_unknown_polarity_is_tolerated);
    RUN_TEST(test_sustained_unknown_polarity_faults);
    RUN_TEST(test_hv_lost_running_faults);
    RUN_TEST(test_wrong_supply_active_faults);
    RUN_TEST(test_supply_fault_when_enabled_faults);
    RUN_TEST(test_safety_fault_beats_manual_abort);
    RUN_TEST(test_safety_fault_beats_timer_completion);
    RUN_TEST(test_stopping_waits_for_hv_off_and_times_out);
    RUN_TEST(test_stopping_still_faults_on_interlock_while_hv_present);
    RUN_TEST(test_fault_entered_with_switch_on_clears_after_off_event_and_hv_off);
    RUN_TEST(test_fault_entered_with_switch_off_requires_off_on_off_interaction);
    RUN_TEST(test_fault_reset_with_interlock_open_goes_interlock_open);
    RUN_TEST(test_output_whitelist_never_enables_in_safe_states);
    RUN_TEST(test_operator_abort_with_immediate_hv_loss_is_aborted);
    RUN_TEST(test_raw_switch_off_prevents_false_hv_loss_fault);
    return UNITY_END();
}
