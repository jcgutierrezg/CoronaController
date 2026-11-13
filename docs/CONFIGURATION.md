# Configuration

Status: values below are from the inspected source, not inferred defaults from older examples. No settings were changed while preparing these docs.

## Active-high PCB update

In `src/main.cpp`, `makeInputCalibration()` currently contains:

```cpp
calibration.hvOnActiveLow = false;
calibration.useFaultInputs = false;
calibration.faultActiveLow = false;
calibration.runSwitchDebounceMs = 50;
```

The later PCB observation supersedes the old active-low interpretation:

| Signal at ESP32 input | LOW | HIGH |
|---|---|---|
| HV_ON | HV reported OFF | HV reported ON |
| FAULT | No fault reported | Fault reported, once decoding is enabled |

The source comment saying `OFF = HIGH, ON = LOW` above the HV_ON assignment is stale. The executable assignment is already correct for active-high behavior. `InputCalibration`'s member defaults in `InputProcessor.h` still use active-low, but production overrides them in the factory above.

`makeControllerConfig()` currently sets:

```cpp
config.useSupplyFaultInputs = false;
```

To activate supply-FAULT handling after wiring and asserted/deasserted levels have been verified, set **both** `calibration.useFaultInputs` and `config.useSupplyFaultInputs` to `true`, retaining `faultActiveLow = false`. The first switch decodes the pins; the second makes the controller react to decoded faults in its enabled-state checks. This is a documented future configuration change, not the current setting.

Earlier advice referenced `HardwareFeatures::NEGATIVE_FAULT_WIRED` and `CABLE_DETECTOR_INSTALLED`. **Neither exists in the inspected `Pins.h`.** Hardware currently reads both FAULT pins unconditionally; do not add or search for those flags as if they were current controls.

## Input calibration in `src/main.cpp`

| Setting | Current value | Status |
|---|---|---|
| `interlockOpen` | Inclusive ADC 0–200 | Configured; PCB measurement record still needed |
| `selectorPositive` | Inclusive ADC 800–1600 | Configured; differs from historical examples |
| `selectorNegative` | Inclusive ADC 1900–2650 | Configured; differs from historical examples |
| Timer ADC range | 0–4095 | 12-bit convention |
| Timer duration | 5–120 seconds | Linear integer mapping, clamped |
| Run-switch debounce | 50 ms | Stable ON/OFF events |
| Voltage-monitor tables | Separate POS/NEG arrays | Recorded points through 15.3 kV |
| Current-monitor tables | Empty `LookupTable{}` | TODO |
| Target tables | Four assigned endpoint-only arrays | Placeholder estimates; TODO empirical calibration |

Run-switch electrical HIGH is interpreted directly as ON; there is no independent run-switch inversion option. Unknown selector gaps produce `selectedPolarity = UNKNOWN` with `interlockClosed = true`; only the explicit open window reports the interlock open.

## Other constants

| Location | Name | Current value |
|---|---|---|
| `lib/Controller/Controller.cpp` | `HV_START_TIMEOUT_MS` | 500 ms, needs measurement |
| Same | `HV_STOP_TIMEOUT_MS` | 500 ms, needs measurement |
| Same | `ABORTED_DISPLAY_TIME_MS` | 2500 ms |
| Same | `POLARITY_UNKNOWN_GRACE_MS` | 100 ms, needs measurement |
| `lib/Hardware/Hardware.cpp` | `FAST_ADC_SAMPLES` | 250 |
| Same | `SLOW_ADC_SAMPLES` | 100 |
| `src/main.cpp` | Slow/display/debug periods | 100 ms each |
| Same, diagnostic branch | `PRINT_INTERVAL_MS` | 250 ms |

Pins are centralized in `lib/Hardware/Pins.h`; see [HARDWARE.md](HARDWARE.md). LCD colors/text are in `lib/Display/Display.cpp`; see [DISPLAY.md](DISPLAY.md). `Hardware::begin()` sets 12-bit ADC resolution but does not explicitly configure attenuation.

## PlatformIO environments

| Environment | Actual configuration / purpose |
|---|---|
| `nodemcu-32s` | `espressif32`, `nodemcu-32s`, Arduino, serial 115200 |
| `diagnostic` | Same target plus `-D DIAGNOSTIC_MODE` |
| `native` | Native platform, Unity, `test_build_src = no`, ignores Hardware/Display, `build_src_filter = -<*>` |

Both ESP32 environments depend on `https://github.com/Seeed-Studio/Grove_LCD_RGB_Backlight.git`. The platform and library reference are not version-pinned in this snapshot; record versions when establishing a reproducible release.

Use `pio test -e native` for host tests. A native firmware Build is not the test workflow. PlatformIO explains [test_build_src](https://docs.platformio.org/en/latest/projectconf/sections/env/options/test/test_build_src.html) and [source filtering](https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_src_filter.html). The proposed split into separate application source files would require revisiting source selection; it is not needed for the current single-file `#ifdef` structure.
