# Troubleshooting record

This collects problems encountered in the project and checks grounded in the present code. It does not replace [SAFETY.md](SAFETY.md) or the laboratory's hardware commissioning procedure.

## Operator OFF produces FAULT instead of ABORTED

The identified race was physical OFF removing HV before the 50 ms switch debounce completed. The old controller treated HV disappearance with stable switch still ON as unexpected loss.

Current fix: `Inputs::rawRunSwitchOn`, populated in InputProcessor begin/update, is checked before active-supply HV loss in RUNNING. Raw OFF after enable has been commanded goes through STOPPING to ABORTED. Interlock, real polarity change, wrong supply, and configured supply fault retain priority in RUNNING. Both controller race regressions now pass. See [STATE_MACHINE.md](STATE_MACHINE.md).

If reproducing the issue, capture raw and debounced switch values alongside HV_ON; normal production serial currently prints only debounced switch. Check that the deployed build includes the source fix rather than assuming a successful host test proves the device firmware was updated.

## Many tests fail after adding the raw switch field

The branch reported 21 failures. Helpers that set `runSwitchOn = true` but left `rawRunSwitchOn = false` simulated physical OFF. Synchronize both fields for settled switch positions; intentionally separate them only for debounce/race cases. The local `validPositiveInputs()` and `requestStart()` helpers now do this. All 30 controller tests passed for this snapshot.

## HV_ON looks inverted, or startup immediately reports HV already ON

Check `src/main.cpp` / `makeInputCalibration()`, not hard-coded inversions in Controller. The assembled PCB was reported active-high; current executable setting is `hvOnActiveLow = false`. The nearby active-low comment is stale. Confirm actual pin levels with diagnostic firmware and correct interface wiring. Synthetic RawInputs defaults are HIGH, so tests constructing them for active-high semantics should explicitly set both HV_ON levels.

## FAULT pin changes but no controller fault occurs

Current production disables both `calibration.useFaultInputs` and `config.useSupplyFaultInputs`. Both must be enabled after commissioning; `faultActiveLow = false` alone only specifies polarity. GPIO16 is already acquired by Hardware. The older `NEGATIVE_FAULT_WIRED` flag is absent from this source. See [CONFIGURATION.md](CONFIGURATION.md) and the reset/start review items before enabling protection.

## Closing the door resumes a treatment

That was superseded behavior. Current energized interlock handling latches FAULT, and door closure alone cannot clear it. INTERLOCK_OPEN additionally requires switch OFF before returning to IDLE. Check deployed firmware against the current source and run the existing no-resume regression. STARTING simultaneous-event priority is still a separate review item.

## Relay activation causes a polarity fault

The owner reported needing 250 fast selector samples instead of 8. Current Hardware uses 250 plus a 100 ms UNKNOWN grace in the controller. A genuine opposite decoded polarity still faults immediately. Check measured selector windows, wiring, relay supply/grounding/decoupling, and worst-case response time; increasing a filter is not a substitute for documenting the disturbance.

## ESP32 boot depends on current-pot position

The earlier GPIO12 assignment caused flash/boot problems. Current `CURRENT_POT` is GPIO34. Verify physical wiring matches it and consult the original-ESP32 strapping restrictions linked in [HARDWARE.md](HARDWARE.md). Do not reuse the old GPIO12 wiring.

## Enable indicator goes OFF but supply remains enabled

An earlier AC-output SSR used in a DC enable circuit produced this behavior. The owner subsequently confirmed a relay module. Check actual feedback and switching hardware through the approved procedure; an input LED or disabled command does not prove an open contact. STOPPING timing out is designed to expose failure to reach HV_OFF.

## Voltage plateaus at 15.3 kV; current shows TODO; target seems implausible

These match current calibration limits: voltage clamps at the last recorded point, measured-current tables are empty, and target tables contain endpoint placeholders. See [CALIBRATION.md](CALIBRATION.md). A numeric validity flag only proves table availability.

## Compiler/tests pass but IntelliSense shows red highlights

This was previously reported after PlatformIO migration. Select the relevant environment, rebuild the PlatformIO IntelliSense index, reset the C/C++ IntelliSense database if needed, and keep backup files outside active source folders. Compiler diagnostics and actual test failures take precedence over stale editor indexing; do not dismiss a real build failure as an indexing problem. Use native Test for host tests and nodemcu-32s Build for firmware.
