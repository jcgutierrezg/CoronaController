# Live voltage and current calibration

Choose **calibration_voltage** or **calibration_current** in PlatformIO, upload, and open Serial Monitor at **115200 baud**. Both run the normal controller, LCD, timing and interlocks. Diagnostic mode remains separate. Supply-FAULT handling retains its configured setting.

Each mode prints only the relevant three averaged ADC readings every 500 ms, with easy-to-read labels. The pots are shared across supply polarities: there is one voltage target pot and one current target pot. Only the monitor signals are separate for positive and negative supplies. Output contains no CSV, other ADC channels, state numbers or debug variables.

Voltage mode prints this layout (numbers below are illustrative, not measured data):

```text
VOLTAGE CALIBRATION (ADC counts)
Target pot:       1000
Positive monitor: 643
Negative monitor: 0
```

Current mode prints the same three labels under CURRENT CALIBRATION, using the current pot and the two current monitor ADC channels. Values are ADC counts, not calibrated kV/µA readings. Normal ADC samples are reused; serial data is sent in available chunks so the loop does not wait for buffer space.

## Recording measured points

Use the laboratory measurement procedure described in [SAFETY.md](SAFETY.md). Once the controller is RUNNING and the operating point/reference reading are steady, record the relevant ADC reading with the independent multimeter measurement and the active polarity. Use the LCD for state; transient samples should not become steady calibration points.

| Mode / reading | Table in src/main.cpp | Reference units |
|---|---|---|
| Voltage / Positive monitor | POSITIVE_VOLTAGE_MONITOR | kV magnitude |
| Voltage / Negative monitor | NEGATIVE_VOLTAGE_MONITOR | kV magnitude |
| Current / Positive monitor | POSITIVE_CURRENT_MONITOR | µA magnitude |
| Current / Negative monitor | NEGATIVE_CURRENT_MONITOR | µA magnitude |
| Voltage / Target pot | POSITIVE_VOLTAGE_TARGET or NEGATIVE_VOLTAGE_TARGET, according to active supply | kV magnitude |
| Current / Target pot | POSITIVE_CURRENT_TARGET or NEGATIVE_CURRENT_TARGET, according to active supply | µA magnitude |

Target tables are polarity-specific because the resulting output can differ between supplies, even though each target pot is shared. Use monitor ADC for monitor calibration and pot ADC for target calibration. Enter points as CalibrationPoint(adcCount, measuredMagnitude), sorted by increasing ADC with no duplicates. Use measured magnitudes in kV or µA, not existing converted LCD readouts.

The actual current-monitor arrays have a user-requested zero baseline only; they remain unavailable until additional current-monitor calibration points exist. The workbook voltage data and separate voltage-based estimated-current tables are now integrated. Both 4095 rows are retained in Excel; the internal saturation flag remains, but the normal display now shows a ~ target estimate at that ADC code (live monitor when available, otherwise the calibrated midpoint). See [CALIBRATION.md](CALIBRATION.md) for interpolation, estimates, and range handling.

The firmware does not insert measurements into the tables automatically or bypass normal treatment timing.

## Current-monitor ADC range trial

Both current-monitor inputs now explicitly use ADC_0db in Hardware::begin(), while voltage monitors and pots retain their default attenuation. This is a software trial for the weak-signal hypothesis at the reported 15-22 µA operating currents, not confirmation of the electrical cause. Collect current calibration points using this new setting; prior ADC/current pairs from a different attenuation setting are not interchangeable.

The original ESP32's specified ADC_0db measurable range is approximately 100-950 mV. Lower attenuation can yield more useful raw counts but does not guarantee accurate readings below the specified range. Both channels still read zero in the owner’s trial. Actual current sensing was then set aside in favor of voltage-based current estimates. Analog gain/external ADC is optional future work, not a current calibration prerequisite. See [Espressif ADC documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html). This setting applies in normal and both calibration modes, as well as diagnostics.
