# Safety contract and implementation limits

This document records the project's safety-related behavior and remaining verification work. It is not a certification, a complete hardware risk assessment, or an energized-work procedure.

## Physical protection and feedback

The agreed design treats the physical interlock and enable chain as primary protections; firmware, HV_ON supervision, and a future watchdog are supplementary. A disabled GPIO or a supply's HV_OFF report does not establish that stored energy has discharged or that an enclosure is safe to open. General manufacturer guidance emphasizes physical barriers, external interlocks, and stored-energy hazards: [Spellman interlock guidance](https://www.spellmanhv.com/en/Technical-Resources/High-Voltage-Reference-Manual/AN-06-just-jumper-the-external-interlock-why-you-really-shouldn-t), [Spellman SL safety manual](https://www.spellmanhv.com/-/media/en/Technical-Resources/Manuals/SLMAN.pdf). These are general references, not identification or instructions for this project's unspecified HVPS models.

Use the actual supplies' manuals and the laboratory's approved isolation, discharge, and verification procedure. The supplied source does not establish the physical interlock wiring, relay failure behavior, GPIO interface levels, or safe reset/power-loss condition.

## Agreed behavior and implemented protections

| Requirement | Current implementation |
|---|---|
| Default application output disabled | `Hardware::begin()` writes ENABLE LOW early; controller output defaults false |
| Only validated enabled states may command HV | Allow-list in `deriveOutputs()` |
| No start with switch already ON at boot | STARTUP fault |
| Unexpected HV before a legitimate start faults | STARTUP and pre-start validation checks; not a global continuous guard |
| Selected supply confirms HV before treatment timer starts | STARTING handshake, then RUNNING entry timestamp |
| Command OFF does not itself mean HV stopped | STOPPING waits for selected HV feedback; timeout faults |
| Interlock opening while enabled invalidates the run | FAULT checks; no automatic resume on door closure |
| Actual opposite polarity while enabled is unacceptable | Immediate polarity fault |
| Brief UNKNOWN disturbance may be tolerated | 100 ms grace; explicit decoded door-open is separate |
| Wrong supply active or unexpected active HV loss faults | Checks in enabled operation; see limits below |
| Operator OFF is cancellation, even before debounce catches up | Raw switch field and RUNNING precedence fix |
| Fault is latched and reset is deliberate | Post-fault event, final switch OFF, both HV reports OFF |
| Supply-FAULT protection can be enabled | Supported but both enable options remain false |

The 40 passing native tests verify selected software scenarios. They do not test electrical failure modes or certify all input combinations.

## Active-high PCB signals

The owner reported HV_ON and FAULT active-high after assembly. Production settings now reflect that; the adjacent old active-low HV_ON comment is incorrect. Confirm LOW/HIGH readings for both supplies in inactive, active, fault, power-up, and disconnected states before relying on these inputs. A polarity flag cannot compensate for a floating or incorrectly interfaced GPIO.

FAULT decoding and controller response are still disabled. Present firmware must not be described as having operational HVPS FAULT protection merely because `faultActiveLow = false` is set.

## Source-observed review items

These findings describe the inspected code; no corresponding physical incident was established during documentation work.

1. **STOPPING does not prove both supplies OFF.** Wrong-supply detection is inside `if (active->hvOn)`. When selected HV is OFF, the controller may enter COMPLETE/ABORTED even if the other supply reports ON. The agreed feedback principle suggests reviewing this against the intended invariant. Fault reset already checks both supplies.
2. **STARTING cancellation precedes safety checks.** During WAITING_FOR_HV_ON, raw OFF takes the controller to STOPPING before checking interlock, polarity, or wrong supply. If selected HV is already OFF on the next update, STOPPING may not latch a concurrent unsafe event. RUNNING has the stronger fault-first order. Decide and test the STARTING priority explicitly.
3. **Unexpected HV is not globally supervised in standby states.** IDLE, INTERLOCK_OPEN, COMPLETE, and ABORTED do not independently check either HV input. Boot and a later validated start are not equivalent to continuous supervision. Start checks also reject UNKNOWN polarity before reaching HV-present checks.
4. **FAULT pins do not gate reset/start validation.** Reset checks both HV reports OFF, but not the supply-FAULT levels. `checkStartConditions()` also has no fault-pin check. Review expected behavior for a persistent fault before enabling protection.
5. **Cable detection is missing.** Earlier discussion included cable verification and energized cable mismatch faults. The inspected code has no cable input, types, or checks. It provides no cable-protection coverage today.

These are recorded as review work in [TODO.md](TODO.md), without silently altering the firmware or inventing an agreed resolution.

## Timing and measurement limits

“Raw OFF immediately” means the next fast processing cycle. It does not bypass ADC acquisition or slow-path work already executing. Start/stop timeouts are 500 ms and UNKNOWN grace 100 ms, all pending real timing/noise validation. There is no configured task watchdog.

Voltage display clamps at the last calibrated 15.3 kV point and retains a true validity flag above that point. Current monitors are uncalibrated. Assigned target tables are endpoint placeholders. None of these displays should be treated as an independently validated safety limit or proof of zero voltage.

See [HARDWARE.md](HARDWARE.md), [CALIBRATION.md](CALIBRATION.md), and [TESTING.md](TESTING.md) for the corresponding evidence and verification tasks.
