# Outstanding work

Status at 13 November 2026. Unchecked items below remain open. Completion requires evidence appropriate to the item; source presence alone does not establish hardware validation. No firmware changes were made as part of this documentation task.

## Carried forward from the conversation and project TODOs

- [ ] **T01 — Finish supply-FAULT commissioning.** Retain active-high settings. Verify both actual GPIOs and fault/no-fault behavior, startup, persistence, and disconnection. Then deliberately enable `useFaultInputs` and `useSupplyFaultInputs` in `src/main.cpp`, with tests. Current value of both is false. The old `NEGATIVE_FAULT_WIRED` instruction does not apply to current code.
- [ ] **T02 — Resolve the cable-detector requirement.** Earlier design included polarity/disconnected/unknown cable states and start/energized checks. Current source contains none. Establish whether omission was intentional; restore the feature or explicitly retire it. Reconsider tentative GPIO15 allocation if restored. Do not describe it as implemented but disabled.
- [ ] **T03 — Calibrate measured current.** Create separate positive/negative ADC-to-µA monitor tables, assign them, and verify display/readback.
- [ ] **T04 — Replace target placeholders.** Measure expected loaded output versus pot ADC for each polarity's voltage/current control. Commented candidate points are not active calibration. Ensure “valid” presentation reflects empirical status.
- [ ] **T05 — Extend measured-voltage coverage.** Add measured points above 15.3 kV if required. Decide how to indicate over-range instead of silently treating the endpoint clamp as a physical limit.
- [ ] **T06 — Measure/tune timing.** Validate 500 ms start/stop timeouts and 100 ms UNKNOWN grace against both real supplies and relay/selector disturbances.
- [ ] **T07 — Reassess noise and response latency on the PCB.** The owner previously needed 250 fast samples instead of 8. Measure worst-case acquisition/control latency and improve grounding/decoupling/relay supply behavior before reducing filtering.
- [ ] **T08 — Add task-watchdog hardening.** Deferred until functional behavior is stable; define timeout and reset behavior without replacing physical protections.
- [ ] **T09 — Verify pot direction and selector calibration.** Confirm CCW minimum/CW maximum, actual PCB windows, ADC configuration, and repeatability for the final wiring.
- [ ] **T10 — Complete physical commissioning records.** Pin/interface truth tables, relay/enable/interlock drawing, PCB revision, supply manuals, reset/power-loss behavior, and bench results remain to be collected.

## Source-review findings from documentation preparation

These are identified differences or questions, not newly agreed implementation changes.

- [ ] **R01 — STOPPING completion must be reviewed.** Current exit checks selected HV OFF; wrong-supply detection only runs while selected HV is ON. Define/test the desired both-supplies-OFF result invariant.
- [ ] **R02 — STARTING simultaneous-event priority.** WAITING_FOR_HV_ON currently handles raw OFF before safety checks. Decide/test concurrent door-open, polarity change, wrong supply, and fault behavior, including selected HV already OFF.
- [ ] **R03 — Unexpected HV in non-running states.** Current HV-present checks occur at startup and pre-start, not continuously in IDLE/INTERLOCK_OPEN/COMPLETE/ABORTED. Review the original unexpected-HV safety requirement and start-check ordering when polarity is UNKNOWN.
- [ ] **R04 — Persistent supply FAULT during reset/start.** Reset does not check fault deassertion, nor does start validation precheck FAULT. Resolve expected behavior before enabling those inputs.
- [ ] **R05 — Calibration validity semantics.** `tableAvailable()` only checks pointer/count. Target placeholders and clamped out-of-range readings remain “valid.” Decide whether empirical/over-range flags and table validation are needed.

## Documentation and regression follow-up

- [ ] **D01 — Correct stale source comments/TODOs.** The HV_ON active-low comment contradicts the active-high assignment; README_TODO mentions a hardware feature flag that does not exist. These docs explain the discrepancy without modifying source.
- [ ] **D02 — Add missing raw InputProcessor regression.** Verify immediate raw OFF, delayed debounced OFF, and one-cycle OFF event. It was proposed in the branch but is absent locally.
- [ ] **D03 — Cover production active-high configuration and PCB windows.** Existing input tests use synthetic active-low calibration and historical selector windows.
- [ ] **D04 — Broaden meaningful safety regressions.** Negative-polarity symmetry, STARTING/STOPPING concurrency, both-HV-OFF results, persistent faults, boundary times, rollover, and all-state output permission. See [TESTING.md](TESTING.md).
- [ ] **D05 — Record reproducible build versions and fresh ESP32 builds.** Platform and LCD dependency are unpinned. This documentation verified host tests only.

Optional architecture improvement: split the two application branches into MainApp/DiagnosticApp files and configure source selection, if that improves maintainability. Current compile-time selection already avoids renaming files.

## Already resolved in the inspected source

- [x] Switch active-high HV_ON/FAULT decoding flags in `makeInputCalibration()`.
- [x] Carry raw run-switch state alongside debounced state/events.
- [x] Handle raw OFF before RUNNING active-HV-loss detection.
- [x] Synchronize settled-switch controller test fixtures and add two controller race regressions.
- [x] Add startup HV-present check and retain energized checks in STOPPING for present supported signals.
- [x] Distinguish COMPLETE/ABORTED/FAULT; prevent interlock closure from resuming a run.
- [x] Select diagnostic mode through a PlatformIO compile-time environment.

The existing 40 host tests passed during documentation preparation; this does not close the open hardware and missing-coverage items above.
