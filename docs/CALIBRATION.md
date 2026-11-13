# Calibration

Status: exact active source values are reproduced below. “Recorded” means present as measured calibration data in the project; no new physical calibration was performed for these docs.

## Selector/interlock windows

`makeInputCalibration()` in `src/main.cpp` currently sets inclusive windows:

| ADC count | Semantic result |
|---|---|
| 0–200 | `interlockClosed = false`, polarity UNKNOWN |
| 800–1600 | Closed, POSITIVE |
| 1900–2650 | Closed, NEGATIVE |
| All gaps/outside windows | Closed, UNKNOWN |

Historical breadboard notes recorded roughly open 0, negative 1350–1460, positive 2660–2830, with example windows 0–100 / 1200–1600 / 2500–2950. Those are **historical**, not current production settings. The InputProcessor unit-test fixture still intentionally uses those older example windows; passing it does not validate the current PCB windows.

The design decision is to use bounded windows and UNKNOWN gaps rather than guess a polarity with one threshold. Record new measurements for both selector positions, door-open, transitions, and relay switching on the final PCB before certifying the configured ranges.

## ADC acquisition and pot convention

`Hardware::begin()` requests 12-bit resolution. Fast selector acquisition averages 250 fresh samples; each slow ADC channel averages 100. Accumulation is local to the channel. There is no shared cross-channel history. The required 250 fast samples are a reported solution to the earlier relay transient, not a universal optimum or a measured response-time guarantee.

Intended pot direction is CCW = ADC zero/minimum and CW = maximum. Hardware correction was an explicit TODO; source code alone does not confirm that every pot now follows it. No explicit ADC attenuation setting is present.

## Timer mapping

ADC 0 maps to 5 s and 4095 maps to 120 s, with integer linear interpolation and endpoint clamping. At ADC 2048 the current mapping yields 62 s. Invalid ADC range configuration falls back to `secondsMinimum`.

The timer is refreshed on the slow path but captured into `RunConfig::durationMs` only after successful start validation. Countdown starts when HV_ON confirms the selected supply. Voltage/current pots remain live.

## Measured-voltage lookup tables

Active points in `src/main.cpp`, units kV:

| Point | Positive ADC | Positive kV | Negative ADC | Negative kV |
|---|---:|---:|---:|---:|
| 1 | 0 | 0.0 | 0 | 0.0 |
| 2 | 195 | 5.0 | 203 | 5.0 |
| 3 | 643 | 10.0 | 663 | 10.0 |
| 4 | 1137 | 15.3 | 1189 | 15.3 |

Positive and negative supplies are deliberately calibrated separately. Monitor values are magnitudes; display polarity is provided separately.

`interpolateCalibration()` linearly interpolates between neighboring ADC points and clamps below/above the endpoints. Thus readings above the final voltage point still display 15.3 kV, with `measuredVoltageValid = true`. This validity flag means a table exists, **not** that a reading is in the measured range. No over-range indicator or extrapolation is implemented.

Extend calibration above 15.3 kV if that range is used; do not interpret the clamp as a physical voltage limit.

## Target estimates: placeholders currently active

The four arrays contain commented intermediate candidate points, but only these endpoints are compiled:

| Active table | ADC 0 | ADC 4095 | Current interpretation |
|---|---:|---:|---|
| `POSITIVE_VOLTAGE_TARGET` | 0.0 kV | 29.8 kV | Endpoint-only estimate |
| `NEGATIVE_VOLTAGE_TARGET` | 0.0 kV | 30.0 kV | Endpoint-only estimate |
| `POSITIVE_CURRENT_TARGET` | 0.0 µA | 999.9 µA | Endpoint-only estimate |
| `NEGATIVE_CURRENT_TARGET` | 0.0 µA | 999.9 µA | Endpoint-only estimate |

All four are assigned to `InputCalibration`. Since two points satisfy `tableAvailable()`, their target-valid flags become true for a known selected polarity. The LCD can therefore display numeric targets despite the remaining calibration TODO. Commented points have no runtime effect and are not an approved empirical table merely because they appear in the file.

The agreed final meaning is **expected real output for the loaded corona setup at that pot position**, not just a nominal fraction of a supply's 0–30 kV or 0–1 mA range. Target values are operator estimates. Measured monitor channels are the treatment readback; neither target estimation nor display conversion implements regulation.

## Measured current

`positiveCurrentMonitor` and `negativeCurrentMonitor` are empty `LookupTable{}` values. Their validity flags remain false. No empirically calibrated measured-current display exists yet; RUNNING normally shows voltage with `I:TODO`.

## Completing a table

Use the laboratory's approved measurement procedure and suitable instrumentation. For each supply and each target/monitor channel, record raw averaged ADC, independently measured physical value, units, reference instrument, PCB/firmware revision, load/geometry, and repeatability. Acquire enough points to describe nonlinear regions.

Keep points in strictly increasing ADC order with no duplicate counts. Current code only checks for a non-null pointer and at least two entries; it does not validate ordering, duplicates, or empirical quality. `LookupTable::count` is an 8-bit field. Attach the array and correct count in `makeInputCalibration()`, then verify interpolation, endpoints, and presentation.

For target estimates, characterize the intended operating load and both polarities separately. For voltage monitoring, retain measured provenance rather than extending the last slope without evidence. Add explicit out-of-range/uncalibrated behavior if needed; that is a review proposal, not implemented behavior.

See [TESTING.md](TESTING.md) for required regressions and [TODO.md](TODO.md) for outstanding calibration tasks.
