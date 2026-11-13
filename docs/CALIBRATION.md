# Calibration and estimated current

## Current data

The active empirical voltage and estimated-current tables in src/main.cpp now come from [Calibration.xlsx](../Calibration.xlsx), sheet 13-11-2026. The owner corrected the negative voltage target-pot typo to 2457 (now F15 after the baseline insertion). A user-requested 0,0,0 baseline was added to row 3 of every Excel dataset block, with matching (0, 0.0f) firmware entries. The zero entries are specified baselines, not additional independently recorded measurements.

Workbook blocks are B:D positive voltage, F:H negative voltage, J:L positive current versus voltage setting, and N:P negative current versus voltage setting. In each block, the first column is the shared voltage-pot ADC and the second is the corresponding supply's voltage-monitor ADC. The current blocks do not contain usable current-monitor ADC readings. Multimeter voltage numbers are interpreted as kV magnitudes for this 30 kV setup; current values are µA magnitudes. Original worksheet headings/layout and all recorded measurements were retained.

| Firmware table | Points | ADC range | Units / purpose |
|---|---:|---|---|
| `POSITIVE_VOLTAGE_TARGET` | 19 | 0–4095 | kV |
| `NEGATIVE_VOLTAGE_TARGET` | 19 | 0–4095 | kV |
| `POSITIVE_VOLTAGE_MONITOR` | 20 | 0–2804 | kV |
| `NEGATIVE_VOLTAGE_MONITOR` | 20 | 0–2809 | kV |
| `POSITIVE_CURRENT_TARGET_ESTIMATE` | 12 | 0–4095 | µA estimate from voltage-pot ADC |
| `NEGATIVE_CURRENT_TARGET_ESTIMATE` | 13 | 0–4095 | µA estimate from voltage-pot ADC |
| `POSITIVE_CURRENT_ESTIMATE` | 13 | 0–2801 | µA estimate from voltage-monitor ADC |
| `NEGATIVE_CURRENT_ESTIMATE` | 14 | 0–2807 | µA estimate from voltage-monitor ADC |

The actual POSITIVE_CURRENT_MONITOR and NEGATIVE_CURRENT_MONITOR arrays each have only a zero baseline. One point is insufficient to enable measured current readback. They are separate from all current-estimate tables.

## Saturated pot readings

Excel retains both 4095 rows: first ADC saturation and physical knob maximum. A lookup cannot distinguish the positions from that ADC code. Firmware target tables include the first 4095 point only, keeping strictly increasing unique keys. At voltagePotAdc >= 4095, target values are marked unavailable and IDLE displays TARGET SATURATED rather than asserting a single output.

Both end-position observations are retained in voltage-monitor-based tables because their monitor ADC readings are distinct. Actual voltage readback reaches 29.7 kV for each supply in the new data. Positive and negative running current estimates extend through 15.9 µA and 22.6 µA respectively, at their last recorded voltage-monitor points.

## Current estimate integration

In IDLE, expected current comes from the voltage-pot ADC and selected polarity. During RUNNING, estimatedCurrentUa comes from the active run's voltage-monitor ADC and polarity; moving the selector does not switch the run's measurement interpretation. The LCD shows I~ for estimated current. If a calibrated actual current-monitor table becomes available later, the LCD prefers the real measurement and omits the estimate marker.

The current-limit pot must stay at the maximum used during collection, with the same geometry, spacing, load and other relevant conditions. The estimate does not model current-limit adjustments or detect changes in the actual load. These values are display estimates, not current feedback or inputs to protection/control decisions.

## Interpolation, range and zero

Values are linearly interpolated between recorded points, including the requested zero baseline. No extrapolation is used for current estimates. Out-of-range estimates show I~-- when voltage remains available. Production enables strict voltage calibration ranges, so voltage outside its recorded table is unavailable instead of silently clamped. IDLE shows OUT OF CAL RANGE for an out-of-range voltage target, and RUNNING shows V OUT OF RANGE for voltage readback outside calibration.

Zero ADC now returns zero voltage and estimated current. Target-pot saturation still overrides target availability at 4095. Actual current monitor validity remains false until each monitor table has at least two entries. The generic InputProcessor retains its legacy clamp behavior unless requireVoltageCalibrationRange is enabled; production enables it.

## Acquisition and other calibration

ADC resolution is 12 bits. Each slow channel averages 100 fresh samples, and the fast selector averages 250. Current-monitor inputs use ADC_0db as the earlier weak-signal trial; voltage monitors and pots retain default attenuation. Monitor-current ADC tables must be collected using that current-channel setting if implemented later. Voltage-based current estimates use the voltage-monitor channels and are independent of the current-channel attenuation.

Current selector windows remain inclusive: open 0–200, positive 800–1600, negative 1900–2650. Gaps mean UNKNOWN polarity with the semantic interlock closed. The timer maps ADC 0–4095 to 5–120 seconds and is captured at successful start validation; countdown begins at HV confirmation. Pot convention is CCW minimum, CW maximum.

Use [LIVE_CALIBRATION.md](LIVE_CALIBRATION.md) for the two simple serial calibration modes. Keep data separate by polarity and record changes in setup. Rebuild and upload after changing tables. The workbook was updated and checked for preserved measurement values; no new physical measurements were performed by the assistant.

Workbook SHA-256 at integration: e1b2defa17fec134cb09d616404abd9721071622e4a1c25cae768fd3ab525079
