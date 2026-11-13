# Hardware and PCB interface

The pin map is verified from source. Electrical observations are owner-reported unless stated otherwise. No PCB schematic, HVPS model-specific manual, or fresh bench measurements were supplied for this documentation.

## Current GPIO allocation

Source: `lib/Hardware/Pins.h`.

| Symbol | GPIO | Firmware direction / role |
|---|---:|---|
| `ENABLE` | 23 | Output; HIGH commands enable, LOW disables |
| `POLARITY_SELECTOR` | 4 | ADC input; combined selector/interlock network |
| `RUN_SWITCH` | 32 | Digital input; HIGH = switch ON |
| `VOLTAGE_POT` | 13 | ADC input |
| `CURRENT_POT` | 34 | ADC input; moved away from GPIO12 |
| `TIMER_POT` | 14 | ADC input |
| `POSITIVE_VOLTAGE_MONITOR` | 27 | ADC input |
| `POSITIVE_CURRENT_MONITOR` | 26 | ADC input |
| `NEGATIVE_VOLTAGE_MONITOR` | 25 | ADC input |
| `NEGATIVE_CURRENT_MONITOR` | 33 | ADC input |
| `POSITIVE_HV_ON` | 36 | Digital input |
| `NEGATIVE_HV_ON` | 35 | Digital input |
| `POSITIVE_FAULT` | 39 | Digital input; decoded faults disabled |
| `NEGATIVE_FAULT` | 16 | Digital input; decoded faults disabled |

All input pins are configured as `INPUT`; firmware does not request internal pull-ups or pull-downs. The LCD uses the `rgb_lcd` library; no explicit I²C pin override appears in project code. Confirm actual board wiring and library defaults rather than assigning LCD pins from this table.

For the original ESP32, GPIO34–39 are input-only without software pull resistors, and GPIO12/MTDI and GPIO15/MTDO participate in boot strapping. These restrictions are documented by [Espressif's ESP32 datasheet](https://documentation.espressif.com/esp32_datasheet_en.html). They should not be generalized to every ESP32-family variant.

## Verified-by-owner changes versus source settings

| Item | Evidence / current state |
|---|---|
| HV_ON polarity | Later assembled-PCB report: active-high; production factory sets `hvOnActiveLow = false` |
| FAULT polarity | Same later report: active-high; factory sets `faultActiveLow = false` |
| FAULT protection enabled? | No: both decoding and controller-response options are false |
| Negative FAULT GPIO acquisition | GPIO16 is configured/read unconditionally in current Hardware code; this does not prove external wiring |
| Enable switching | Owner confirmed use of a relay module |
| Selector noise | Owner reported 250 ADC samples were needed where 8 had failed |
| Pot direction | Intended CCW = zero/minimum, CW = maximum; source notes hardware correction/calibration remains to verify |

The earlier fault-signal ambiguity is historical context. The later active-high discovery resolves the configured polarity, but is not a complete record of both fault inputs' wiring, startup behavior, fault persistence, or fail-disconnected behavior. See [CONFIGURATION.md](CONFIGURATION.md).

## Selector and interlock ADC

The present production windows are 0–200 open, 800–1600 positive, and 1900–2650 negative, inclusive. Values in the gaps mean unknown polarity, not an estimated positive/negative selection. They currently leave the semantic interlock closed. Historical breadboard notes used different and differently assigned windows; do not copy those into the PCB configuration. See [CALIBRATION.md](CALIBRATION.md).

There is no separate digital interlock input in the current pin map. The physical safety-chain wiring is not established by the software decoder and needs its own documented verification.

## GPIO12 boot failure

The earlier current-pot connection to GPIO12 caused flash-detection/reboot behavior that depended on pot position at boot. The agreed remedy was moving it away from the strap pin; the current source uses GPIO34. GPIO15 was a tentative cable-detector pin in the old discussion; cable detection has now been explicitly discarded. No allocation for it is required.

## Relay and earlier SSR failure

The discussion recorded an AC-output SSR used in a DC enable circuit: its input indicator went OFF while the supply remained enabled, and opening the physical circuit stopped it. An AC triac/thyristor SSR's inability to reset on a DC load is consistent with [OMRON's explanation](https://www.ia.omron.com/support/faq/answer/18/faq02101/). The exact original SSR internals were not independently inspected.

The owner subsequently confirmed a relay module. Its model, contact rating for the actual DC circuit, driver/flyback provisions, contact topology, and reset/power-loss behavior still need to be recorded. ENABLE LOW is the firmware's disabled command; it is not proof that the relay contacts are open.

## Hardware records still needed

- PCB revision and schematic, interface voltage limits, level-shifting/isolation, signal reference, and pull/bias circuitry.
- HVPS models and manuals; signal truth tables for both supplies, including power-up and disconnected inputs.
- Enable-chain/interlock drawing and measured behavior during MCU reset, boot, loss of power, or stalled execution.
- Relay model and DC switching suitability; selector transient and startup/shutdown timing logs.
- Final selector ADC ranges, pot direction, monitor scaling, and calibration conditions.

See [SAFETY.md](SAFETY.md) for boundaries of the software's feedback assumptions.

The owner corrected the GPIO25/26 allocation during voltage calibration: positive current is GPIO26 and negative voltage is GPIO25. Raw current monitors still returned zero after ADC_0db was tried on GPIO26/33; weak-signal amplitude remains a working explanation, not an independently established hardware diagnosis. Current estimation now uses the voltage monitors.
