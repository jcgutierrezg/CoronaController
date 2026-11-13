# Design decisions and superseding changes

This record combines the accessible consolidated design discussion, later branch corrections, and inspected implementation. It is a decision history, not a claim that every proposal was implemented or bench-verified. Provenance and limitations are in [SOURCES.md](SOURCES.md).

## 1. Layer the firmware and migrate to PlatformIO

Decision: separate electrical access, interpretation, machine sequencing, presentation, and orchestration. Use native Unity tests for hardware-independent behavior and an ESP32 environment for firmware.

Current: Hardware, InputProcessor, Controller, Display, and main.cpp implement that division. Headers expose interfaces/types; source files provide behavior. Scoped enums and typed constants keep states, faults, and pin assignments explicit. Hardware-specific libraries are excluded from native tests.

## 2. Model machine history explicitly

Decision: a maintained switch alone cannot represent machine state. Use STARTUP, IDLE, STARTING, RUNNING, STOPPING, COMPLETE, ABORTED, INTERLOCK_OPEN, and FAULT. STARTING has VALIDATING and WAITING_FOR_HV_ON phases.

Current: all nine states and both phases exist. A missing STARTING dispatcher case was an earlier test-discovered defect; the current dispatcher includes it.

## 3. Separate successful completion, cancellation, readiness, and faults

Decision: timer completion is COMPLETE; operator early stop is ABORTED; door open while idle is INTERLOCK_OPEN; safety-invalidating energized conditions latch FAULT. Closing the door must not resume an interrupted run.

Current: implemented. ABORTED has a 2500 ms minimum presentation interval plus switch-OFF acknowledgement; COMPLETE waits for OFF. Door closure with switch still ON cannot exit INTERLOCK_OPEN to restart. Earlier interlock behavior that resumed treatment was superseded.

## 4. Distinguish command from feedback

Decision: command relay ON only after validation, wait for selected HV_ON before RUNNING, and command OFF then wait in STOPPING before presenting a result. Include start/stop timeouts and reject wrong-supply activity.

Current: implemented with 500 ms timeout placeholders pending physical timing measurements. The earlier AC-output SSR failure demonstrated why an input LED or OFF command is insufficient feedback. The owner later confirmed a relay module.

Review: current STOPPING completion only checks selected-supply HV OFF; the broader both-supplies-OFF invariant is not enforced on that exit. See [SAFETY.md](SAFETY.md).

## 5. Capture treatment timing and polarity

Decision: capture timer and polarity at validated start; start elapsed treatment time only at HV confirmation. Voltage/current pot adjustments remain live. Do not block the main loop for a treatment-duration delay.

Current: `RunConfig` stores polarity and duration; RUNNING entry timestamps the timer. Elapsed comparisons use unsigned subtraction. Measurement readback follows captured run polarity where available.

## 6. Latch faults and require deliberate reset

Decision: physical recovery alone is not an acknowledgement. Require switch interaction after fault entry, final OFF, and both supplies reporting HV OFF. A fault entered with switch already OFF requires OFF → ON → OFF. A reset with door open returns to INTERLOCK_OPEN.

Current: implemented and covered by existing tests. The reset condition does not separately require supply-FAULT pins to deassert; review before enabling those inputs.

## 7. Derive output from an allow-list

Decision: only validated STARTING / WAITING_FOR_HV_ON and RUNNING may command enable. Default every other state to disabled; display code does not control relays.

Current: `deriveOutputs()` implements this. Application initialization writes ENABLE LOW first after configuring it as an output. Boot/reset electrical behavior still requires hardware verification.

## 8. Decode ADC state windows and tolerate limited UNKNOWN transients

Decision: selector/interlock values occupy discrete windows; leave gaps UNKNOWN instead of guessing. Reject an actual opposite polarity immediately while energized, but allow a short UNKNOWN disturbance caused by switching.

Current: 100 ms UNKNOWN grace. Historical selector windows were later replaced in local production configuration by open 0–200, positive 800–1600, negative 1900–2650. Exact PCB measurement evidence is still needed. An explicit open-window result is not granted UNKNOWN grace.

## 9. Filter each ADC channel independently

Decision: eliminate the shared rolling buffer that mixed channel histories. Average fresh local samples per channel and keep safety/config acquisition ahead of slow pots and display work.

Later hardware finding: 8 selector samples were insufficient; the owner reported needing 250 to suppress relay-related false polarity behavior.

Current: 250 fast samples and 100 slow samples per channel. Revisit noise and latency after PCB/grounding/decoupling improvements using measurements.

## 10. Use empirical, separate calibration tables

Decision: nonlinear monitor signals need lookup interpolation. Keep positive/negative voltage/current calibration separate. Target-pot readouts should eventually estimate actual loaded corona output rather than nominal supply range.

Current: measured voltage tables through 15.3 kV; deliberate upper clamp. Current-monitor calibration remains absent. Four target tables now exist and are assigned, but only endpoint placeholders are active; their validity flags do not prove calibration. Earlier statements that all target tables were simply missing are no longer a precise description.

## 11. Make supply-input polarity configurable

Earlier interpretation: HV_ON and FAULT were treated as active-low in examples; FAULT handling remained disabled because semantics were ambiguous.

Superseding PCB finding: the owner reported both are active-high after assembly. Current `makeInputCalibration()` sets both ActiveLow flags false. An old HV_ON comment still contradicts the assignment and should be corrected. Enabling FAULT requires two additional true flags; those remain false today.

Earlier advice mentioning `HardwareFeatures::NEGATIVE_FAULT_WIRED` applies to an older design and not the inspected code, which reads GPIO16 unconditionally.

## 12. Separate raw OFF from debounced ON intent

Later defect: physical OFF could remove HV before the maintained-switch debounce completed, producing a false HV_LOST fault.

Decision: preserve a raw run-switch field as well as stable switch/events. Start on debounced ON; act on raw OFF once enable has been commanded. VALIDATING continues to use the debounced value.

Current: implemented; RUNNING cancellation now precedes HV-loss detection while interlock/polarity/wrong-supply/configured-fault checks retain priority. Two regression tests pass. Updating fixture helpers resolved the mismatch in which tests set stable ON but left raw OFF; this was the explanation for the reported 21 failing tests. STARTING simultaneous-event precedence still merits regression coverage.

## 13. Keep unfinished features explicit

Earlier agreement: cable-detection architecture and fault-input support should be configurable while hardware work proceeds; watchdog hardening was postponed until behavior stabilized.

Current: fault support remains configurable; watchdog is a TODO. Cable types, pin, checks, and feature switches are **absent** in the supplied source. Available history does not establish whether that removal was intentional. Restore or explicitly retire the requirement rather than describing it as implemented-but-disabled.

GPIO15 was only a tentative cable pin and requires reconsideration because of strapping. The current-pot boot problem was resolved in the pin allocation by using GPIO34 instead of GPIO12.

## 14. Use clear LCD state feedback and separate diagnostic firmware

Decision: state colors, fixed-width LCD updates, and a diagnostic environment are preferable to repeated LCD clearing or renaming main/debug files.

Current: [DISPLAY.md](DISPLAY.md) records the exact colors/screens. `DIAGNOSTIC_MODE` selects raw-input diagnostics in main.cpp and commands enable LOW every loop. The proposed separate MainApp/DiagnosticApp source split remains optional architecture work; it is not the present layout.
