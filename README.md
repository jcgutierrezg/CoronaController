# CoronaController

ESP32 / Arduino firmware for a corona-treatment controller, built with PlatformIO. The controller supervises a maintained run switch, polarity/interlock selector, two HV supplies, treatment timing, an enable relay, and a 16×2 RGB LCD.

Documentation snapshot: **13 November 2026**. Checked against the source folder supplied by the owner and the accessible design conversations. This package contains documentation only; it does not change the firmware.

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

**The assembled PCB's HV_ON and FAULT signals are active-high**, as subsequently reported by the owner. In `src/main.cpp`, `makeInputCalibration()` now sets `hvOnActiveLow = false` and `faultActiveLow = false`. An adjacent active-low HV_ON comment is stale. Supply-FAULT decoding and controller response are **both still disabled**; polarity configuration alone does not enable protection.

The raw/debounced switch race fix is implemented. Raw OFF is checked before unexpected active-supply HV loss during `RUNNING`; two controller regression tests pass. Stable ON requests still use debounce. See [STATE_MACHINE.md](docs/STATE_MACHINE.md).

Voltage-monitor tables contain recorded points through **15.3 kV** and clamp above that. Current-monitor tables are empty. Four target tables are assigned but contain only endpoint placeholders, so a displayed target is not evidence of empirical calibration. Cable detection discussed earlier is absent from the inspected source, rather than simply disabled by a feature flag.

## Build and test

Open the firmware project folder in VS Code with PlatformIO, or run these commands there:

```text
pio run -e nodemcu-32s
pio test -e native
pio run -e diagnostic
```

The normal environment uses `espressif32`, `nodemcu-32s`, Arduino, and Seeed's Grove LCD RGB Backlight library. Diagnostic mode is selected with `DIAGNOSTIC_MODE`. See [CONFIGURATION.md](docs/CONFIGURATION.md) and [DIAGNOSTICS.md](docs/DIAGNOSTICS.md). Command semantics: [PlatformIO run](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html), [PlatformIO test](https://docs.platformio.org/en/latest/core/userguide/cmd_test.html).

**Verification for this documentation:** 30 controller tests and 10 InputProcessor tests passed using the installed host C++ compiler and the project's Unity sources. PlatformIO firmware builds, uploads, and physical tests were not performed during documentation preparation.

## Documentation map

| File | Contents |
|---|---|
| [ARCHITECTURE.md](docs/ARCHITECTURE.md) | Layers, data flow, timing, source layout |
| [DESIGN_DECISIONS.md](docs/DESIGN_DECISIONS.md) | Decisions and later superseding changes |
| [HARDWARE.md](docs/HARDWARE.md) | Current GPIO map, PCB signals, relay history |
| [STATE_MACHINE.md](docs/STATE_MACHINE.md) | States, phases, priorities, reset and switch race |
| [CALIBRATION.md](docs/CALIBRATION.md) | Actual tables, windows, placeholders, calibration work |
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
