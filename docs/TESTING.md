# Testing and verification

## Results for this documentation snapshot

On 13 November 2026, after empirical calibration integration and zero baselines, PlatformIO native tests passed **48/48**: 30 controller tests and 18 InputProcessor tests, zero failures/ignored. The input suite includes eight new regressions for current estimates, saturation, range handling and zero behavior.

All four ESP32 environments built successfully: nodemcu-32s, diagnostic, calibration_voltage, calibration_current. Observed build dependencies: Espressif32 platform 7.1.0; Arduino framework package 3.20017.241212 (Arduino core 2.0.17); Xtensa toolchain 8.4.0+2021r2-patch5; Grove LCD RGB Backlight 1.0.2 at dbd5674. Configuration remains unpinned. Builds/tests do not establish live GPIO/LCD/physical safety performance. No assistant upload or energized measurement was performed.

The initial documentation-only review used direct host compilation and 40 tests. Those figures are historical and superseded by the latest PlatformIO results above.

## Normal project workflow

From the firmware project root:

```text
pio test -e native
pio run -e nodemcu-32s
pio run -e diagnostic
pio run -e calibration_voltage
pio run -e calibration_current
```

The native environment selects Unity and excludes Arduino-dependent Hardware/Display code. Use native **Test**, not native firmware **Build**. Official command references: [pio test](https://docs.platformio.org/en/latest/core/userguide/cmd_test.html), [pio run](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html).

## Existing controller coverage

`test/test_controller/test_controller.cpp` covers:

- Safe startup; switch ON at boot; HV present at boot/pre-start.
- IDLE door-open handling and no restart until door closed plus switch OFF.
- Captured duration/polarity; startup timeout; cancellation before enable.
- Manual stop through STOPPING/ABORTED and timed completion through STOPPING/COMPLETE.
- Countdown beginning at HV confirmation, COMPLETE acknowledgement, ABORTED minimum display time.
- Interlock opening during RUNNING and no resume on closure.
- Immediate opposite-polarity fault, short UNKNOWN tolerance, sustained UNKNOWN fault.
- Unexpected active-HV loss, wrong supply active, configured positive supply FAULT.
- Safety-fault priority over manual abort and timer expiry in RUNNING.
- STOPPING waiting for selected HV OFF, stop timeout, interlock fault while selected HV remains ON.
- Fault acknowledgement when entered switch ON versus switch already OFF, and reset with door open.
- A limited output-allow-list check in safe states.
- Operator OFF with immediate HV loss, including raw OFF while debounced switch remains ON.

Most fixtures are positive-polarity runs. The helper named `fullConfig()` now only disables supply-fault reaction; it does not enable a cable-detection feature.

## Existing InputProcessor coverage

`test/test_input_processor/test_input_processor.cpp` covers discrete selector windows/gaps, active-low HV decoding, ignored/enabled fault decoding, debounced ON events, timer mapping/clamping, voltage interpolation/clamping, measurement-polarity selection, missing-table flags, and adding a target lookup table.

Its calibration is synthetic: historical example selector windows and active-low supply signals. These tests exercise configurability; they do not prove the active-high production factory or current PCB window values. They also do not verify the empirical quality of production target tables.

## Switch-race regression contract

Two existing passing controller tests are particularly relevant:

| Test | Simulated condition | Expected result |
|---|---|---|
| `test_operator_abort_with_immediate_hv_loss_is_aborted` | Raw OFF, debounced OFF, selected HV OFF together | STOPPING with no fault, then ABORTED |
| `test_raw_switch_off_prevents_false_hv_loss_fault` | Raw OFF, debounced still ON, selected HV OFF | Same cancellation result |

For settled switch positions, controller fixtures must set **both** `rawRunSwitchOn` and `runSwitchOn`. `requestStart()` now does so. Leaving raw false while setting only debounced true accidentally models physical OFF; this explained the reported 21 failures after introducing the raw field.

InputProcessor tests should supply raw pin levels and inspect the generated outputs, rather than manually synchronizing semantic fields.

## Missing regressions / source-review checks

These are proposed tests, not passing coverage claims:

- InputProcessor raw OFF updates on the first update while debounced ON and OFF-event remain unchanged; after 50 ms, stable OFF and exactly one OFF event appear. This test was proposed in the branch but is not present in the local file.
- Active-high HV_ON and active-high FAULT truth tables for **both** supplies, including disabled-fault behavior.
- Current production ADC-window boundaries/gaps and the factory's actual polarity assignments.
- STARTING / WAITING_FOR_HV_ON raw-OFF cancellation, plus raw OFF simultaneous with door-open, wrong-supply HV, opposite polarity, or supply FAULT. Clarify intended precedence before changing behavior.
- STOPPING selected HV OFF while the wrong supply reports ON; confirm the intended both-supplies-OFF result condition.
- Unexpected HV in IDLE, INTERLOCK_OPEN, COMPLETE, and ABORTED; persistent supply FAULT during reset/restart.
- Negative-polarity symmetry, exact timeout/grace boundaries, rollover, and output permission across all states/phases.
- Production calibration-table/screen integration and malformed hand-edited tables if runtime table validation is added. Range/saturation and estimated-versus-measured separation are already covered by input regressions.

See the source-observed review items in [TODO.md](TODO.md); existing passing tests do not resolve them.

## Bench validation to perform

Use the laboratory's approved commissioning procedure. Begin with HV generation prevented and verify actual PCB GPIO truth tables, selector ranges, switch wiring, disabled relay command, and boot/reset/power-loss behavior. Diagnostic firmware provides raw readings but cannot prove physical de-energization.

After the hardware safety chain is independently established, record controlled tests of start/stop timing, cancellation during startup and treatment, interlock interruption/no-resume, wrong-supply feedback, configured faults, fault reset, and relay transients for both polarities. Measure worst-case input-to-output latency with ADC averaging and display/serial work active. Do not treat the 500 ms constants or the 250-sample filter as approved measured limits.

Record firmware/source hashes, PCB revision, instrumentation, expected result, actual result, and pass/fail for each bench case. No such new bench evidence is included in this package.

## Empirical integration regressions

New InputProcessor regressions cover voltage-pot-based expected current independent of current-pot position, distinct RUNNING estimates with the pot saturated at 4095, negative polarity selection/interpolation, unavailable out-of-range estimates, saturation affecting targets only, separation from actual current feedback, strict voltage ranges without invented baseline points, and the owner-requested zero baseline. The no-baseline synthetic fixture remains a test of range handling; production tables now include zero.
