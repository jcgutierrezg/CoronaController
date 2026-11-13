# CoronaController

ESP32 / Arduino firmware for a corona-treatment controller, built with PlatformIO. The controller supervises a maintained run switch, polarity/interlock selector, two HV supplies, treatment timing, an enable relay, and a 16×2 RGB LCD.

Documentation snapshot: **13 November 2026**. Checked against the source folder supplied by the owner and the accessible design conversations. The documentation describes the current project, including the calibration/estimate implementation and the owner’s latest scope decisions.

## Read this status correctly

| Label | Meaning |
|---|---|
| Implemented | Present in the inspected source; does not by itself prove physical performance |
| Native-tested | Existing hardware-independent tests were compiled and executed for this snapshot |
| Hardware-reported | Observed/reported by the owner in the conversation; not remeasured for these docs |
| Agreed | Intended behavior recorded in the design discussion |
| TODO / Review | Incomplete work, missing validation, or an identified implementation difference |

See [IMPLEMENTATION_STATUS.md](docs/IMPLEMENTATION_STATUS.md) for the evidence matrix and [SOURCES.md](docs/SOURCES.md) for provenance and retrieval limits.

## Main behavior

- Startup refuses a switch already ON or pre-existing HV.
- A deliberate debounced ON request enters `STARTING`: validate first, then command enable and wait for the selected supply's `HV_ON`.
- Treatment time starts at HV confirmation. Duration and polarity are captured for the run; voltage/current pots remain live.
- Timer expiry goes through `STOPPING` to `COMPLETE`. Operator cancellation goes through `STOPPING` to `ABORTED` once enable has been commanded.
- Opening the interlock during energized operation latches `FAULT`. Closing it cannot resume a treatment.
- Fault reset requires post-fault switch interaction, switch OFF, and both supplies reporting HV OFF.
- Only `STARTING / WAITING_FOR_HV_ON` and `RUNNING` command enable.

## Latest corrections and current limitations

**The assembled PCB's HV_ON and FAULT signals are active-high**, as subsequently reported by the owner. In `src/main.cpp`, `makeInputCalibration()` now sets `hvOnActiveLow = false` and `faultActiveLow = false`. Supply-FAULT decoding and controller response are **both still disabled**; polarity configuration alone does not enable protection.

The raw/debounced switch race fix is implemented. Raw OFF is checked before unexpected active-supply HV loss during `RUNNING`; two controller regression tests pass. Stable ON requests still use debounce. See [STATE_MACHINE.md](docs/STATE_MACHINE.md).

The corrected calibration workbook is integrated through **29.7 kV** for each voltage monitor. Current is estimated from voltage using separate polarity curves and labeled **I~**; actual current sensing remains unavailable. All datasets include the requested zero baseline. At pot ADC 4095, a marked ~ voltage estimate replaces the former saturation message; a valid live monitor in the clipped range is used when HV is on, otherwise the calibrated range midpoint is shown. Production uses strict calibration ranges for measured voltage. These estimates assume the recorded setup and current-limit pot at maximum.

**Scope:** cable detection was explicitly discarded. Supply-FAULT handling remains pending verification, and the watchdog remains planned. The remaining work and controller review findings are in [TODO.md](docs/TODO.md).

## Build and test

Open the firmware project folder in VS Code with PlatformIO, or run these commands there:

```text
pio run -e nodemcu-32s
pio test -e native
pio run -e diagnostic
pio run -e calibration_voltage
pio run -e calibration_current
```

The normal environment uses `espressif32`, `nodemcu-32s`, Arduino, and Seeed's Grove LCD RGB Backlight library. Diagnostic mode is selected with `DIAGNOSTIC_MODE`. See [CONFIGURATION.md](docs/CONFIGURATION.md) and [DIAGNOSTICS.md](docs/DIAGNOSTICS.md). Command semantics: [PlatformIO run](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html), [PlatformIO test](https://docs.platformio.org/en/latest/core/userguide/cmd_test.html).

**Latest verification:** all four ESP32 firmware environments built successfully, and PlatformIO native tests passed **58/58** (30 controller, 28 InputProcessor). The assistant has not uploaded firmware or performed physical measurements.

## Documentation map

| File | Contents |
|---|---|
| [ARCHITECTURE.md](docs/ARCHITECTURE.md) | Layers, data flow, timing, source layout |
| [DESIGN_DECISIONS.md](docs/DESIGN_DECISIONS.md) | Decisions and later superseding changes |
| [HARDWARE.md](docs/HARDWARE.md) | Current GPIO map, PCB signals, relay history |
| [STATE_MACHINE.md](docs/STATE_MACHINE.md) | States, phases, priorities, reset and switch race |
| [CALIBRATION.md](docs/CALIBRATION.md) | Active workbook data, current estimates, range policy |
| [TESTING.md](docs/TESTING.md) | Existing coverage, results, missing regressions, bench checks |
| [CONFIGURATION.md](docs/CONFIGURATION.md) | Exact current settings and where to change them |
| [SAFETY.md](docs/SAFETY.md) | Safety contract and actual implementation limits |
| [DISPLAY.md](docs/DISPLAY.md) | LCD colors, screens, validity handling |
| [DIAGNOSTICS.md](docs/DIAGNOSTICS.md) | Diagnostic firmware and normal serial output |
| [OPERATIONS.md](docs/OPERATIONS.md) | Operator-facing start, stop, result, and acknowledgement behavior |
| [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | Previously encountered failures and remedies |
| [IMPLEMENTATION_STATUS.md](docs/IMPLEMENTATION_STATUS.md) | Agreement versus code versus evidence |
| [TODO.md](docs/TODO.md) | Remaining work, separated by origin |
| [SOURCES.md](docs/SOURCES.md) | Source/conversation provenance and external references |

Read [SAFETY.md](docs/SAFETY.md) before commissioning. HV_OFF feedback is a controller input, not proof of discharged or touch-safe hardware.

## Live calibration firmware

The `calibration_voltage` and `calibration_current` PlatformIO environments run the normal controller while printing only the shared target pot and positive/negative monitor ADC readings for the selected quantity. Serial output uses readable labels at 115200 baud. See [LIVE_CALIBRATION.md](docs/LIVE_CALIBRATION.md).

## Updated empirical calibration

The corrected workbook is now integrated: voltage readback through 29.7 kV, voltage-based current estimates labeled `I~`, and marked target estimates when the pot saturates at ADC 4095. All datasets have the requested zero baseline. See [CALIBRATION.md](docs/CALIBRATION.md). Current estimates assume the recorded setup and current-limit pot at maximum.
