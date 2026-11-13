# Remaining work and scope decisions

Reviewed 13 November 2026 against current code, Calibration.xlsx, build/test results and the owner's explicit decisions. This is the authoritative backlog; README_TODO.txt redirects here. A disabled feature is not automatically a discarded requirement.

## Confirmed scope

Cable detection was explicitly discarded by the owner on 13 November 2026. Supply-FAULT handling remains planned pending verification; the watchdog remains planned as final hardening.

Direct current-monitor calibration has been set aside for the current setup after both channels still read zero with ADC_0db. Voltage-based current estimates replace it for the present workflow. Hardware amplification or an external ADC is an optional future enhancement, not a prerequisite for finishing this version. Actual-current interfaces remain in code with a zero baseline only.

## Open controller reviews

These findings were identified during source review and are not resolved by the existing tests. This documentation update did not change the firmware.

| ID | Remaining decision/work | Current behavior |
|---|---|---|
| R01 | Require/review both supplies OFF before a STOPPING result | Selected HV OFF can complete/abort while wrong-supply checking is only inside selected HV ON |
| R02 | Define STARTING priority for simultaneous raw OFF and safety faults | WAITING_FOR_HV_ON handles raw OFF before safety checks; RUNNING has stronger fault-first priority |
| R03 | Review unexpected HV supervision in non-running states | Checked at startup/pre-start, not continuously in IDLE/INTERLOCK_OPEN/COMPLETE/ABORTED; UNKNOWN polarity rejects before pre-start HV checks |
| R04 | Define persistent supply-FAULT behavior at start/reset | Neither reset nor start validation requires FAULT deassertion; resolve before enabling T01 |

These are the next firmware review items to address; they are not cable-detection work.

## Planned implementation and hardware verification

- [ ] **T01 — Supply-FAULT commissioning.** Keep active-high polarity. Verify both signal interfaces, fault/no-fault levels, power-up, persistence and disconnection; resolve R04; then enable both useFaultInputs and useSupplyFaultInputs with regressions. Both remain false today. GPIO16 is already read; the old NEGATIVE_FAULT_WIRED flag does not exist.
- [ ] **T06 — Measure/tune response times.** Validate 500 ms HV start/stop timeouts and 100 ms UNKNOWN grace for both supplies. Values exist but are not backed by recorded timing measurements.
- [ ] **T07 — PCB noise and loop latency.** Retain 250 selector samples for now. Verify the final PCB's relay transient behavior and worst-case input-to-output response. Reducing averaging is optional if measurements permit it.
- [ ] **T08 — Task watchdog.** Still planned. Add final-hardening configuration, timeout/reset policy, and tests/bench verification after the state-machine review.
- [ ] **T09 — Finish input verification records.** Pot direction, physical maximum/current-limit setting, selector window boundaries and repeatability on the PCB. Voltage calibration is complete for the supplied dataset; do not restart that work as an empty TODO.
- [ ] **T10 — Commissioning evidence.** Record PCB/interface/enable/interlock wiring, supply manuals, relay DC suitability, boot/reset/power-loss behavior and controlled functional checks. Validate the new voltage/current-estimate LCD on the actual device; estimates assume the same setup and maximum current limit.

## Remaining regression coverage

- [ ] **D02 — Raw OFF InputProcessor regression.** Immediate raw OFF, delayed stable OFF, and exactly one OFF event. Controller-level race regressions already pass; the separate input-layer test is still absent.
- [ ] **D03 — Production input configuration.** Active-high HV_ON/FAULT decoding for both supplies and current PCB selector-window boundaries. Existing input fixtures still use synthetic active-low settings.
- [ ] **D04 — Safety edge cases.** Tests accompanying R01–R04, negative-run symmetry, exact boundaries/rollover, and all-state/phase output permission. Negative current-estimate interpolation is now tested; that does not substitute for negative-run safety tests.

## Completed or replaced (not open TODOs)

| Previous ID / idea | Resolution |
|---|---|
| T02 cable detector / tentative GPIO15 | **Discarded explicitly**; do not reintroduce as missing firmware |
| T03 measured-current calibration | **Set aside/replaced for this version** by voltage-based I~ estimates; optional future hardware/readback work |
| T04 target placeholders | Replaced with empirical voltage targets and voltage-based expected-current tables |
| T05 15.3 kV ceiling | Replaced with measured voltage data through 29.7 kV and strict range handling |
| R05 displayed calibration validity | Production range/saturation flags implemented; zero baselines explicit; unavailable estimates do not silently clamp |
| D01 active-low comment / obsolete feature-flag instructions | HV_ON comment corrected; legacy README_TODO replaced with a redirect |
| D05 fresh firmware build evidence | All four firmware environments build; 58 native tests pass. Dependency version pinning remains optional release work |
| Combined calibration CSV | Replaced by voltage/current modes, each with three readable ADC labels |
| Independent positive/negative target pots | Corrected: pots are shared; monitor channels and response curves are polarity-specific |
| Treating two ADC-4095 observations as distinct pot positions | Discarded as unobservable; preserve both in Excel, one lookup key, marked monitor/midpoint estimate |
| Missing STARTING dispatcher, interlock resume, abort/result semantics, switch debounce race | Corrected in current firmware; retain regressions |
| GPIO25/26 assignment mismatch | Owner corrected Pins.h: positive current GPIO26, negative voltage GPIO25 |

## Optional, not release blockers by default

- Pin dependency/platform versions for reproducible releases (observed versions are in TESTING.md).
- Split main.cpp into application files if useful; compile-time environment selection already works.
- Validate arbitrary hand-edited tables for malformed order/duplicate keys; imported production tables have been checked.
- Improve pot ADC headroom to distinguish the final knob travel if precise IDLE targets near maximum become important. Current saturation handling is deliberate.
- Revisit actual current sensing with analog gain/external ADC only if requirements change.

The full historical transcript was not retrievable. Explicit decisions from this chat take precedence over older incomplete summaries. See DESIGN_DECISIONS.md and SOURCES.md.
