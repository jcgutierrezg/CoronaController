# Diagnostic firmware and serial logs

Status: diagnostic branch and environment exist in the inspected source. No firmware was flashed for this documentation task.

## Selecting firmware

`platformio.ini` defines `nodemcu-32s` for the controller and `diagnostic` with `-D DIAGNOSTIC_MODE` for diagnostics. `src/main.cpp` contains one pair of diagnostic setup/loop entry points under `#ifdef`, with production code under `#else`. Do not rename source files to switch modes.

Build the chosen environment from the firmware project root:

```text
pio run -e diagnostic
pio run -e nodemcu-32s
```

To install a chosen build as part of an approved commissioning workflow, use that environment's PlatformIO Upload task. The upload environment must be explicit; these docs did not perform an upload. Open Serial Monitor at **115200 baud**. PlatformIO's environment/target command behavior is documented in [pio run](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html).

## Diagnostic output and enable behavior

Diagnostic setup calls `hardware.begin()` and applies `Outputs{ enableHv = false }`. Every diagnostic loop applies disabled output again before periodic acquisition/printing. GPIO23 is therefore commanded LOW by this branch. Physical contact state, boot-time behavior, and absence of residual HV still require independent verification.

Approximately every 250 ms when the loop reaches the print block, diagnostics reports:

- Raw run-switch HIGH/LOW.
- Averaged selector/interlock ADC.
- Positive and negative HV_ON HIGH/LOW.
- Positive and negative FAULT HIGH/LOW.
- Averaged timer, voltage, and current pot ADCs.
- Averaged positive/negative voltage and current monitor ADCs.

Both FAULT GPIOs are read regardless of the production fault-enable settings. These logs show electrical levels, not decoded fault meaning. “Raw” selector/pot values are already averaged by Hardware; they are not individual ADC conversions. The diagnostic branch does not run the controller or render normal LCD states.

Use these readings to document the assembled PCB's active-high truth table and actual selector ranges. A 250 ms print period is unsuitable for proving short transient timing by itself.

## Production debug output

Production prints approximately every 100 ms:

```text
state=... switch=... interlock=... polarity=... timer=...
enable=... posHV=... negHV=... V=... fault=...
```

`switch` is the **debounced** value; rawRunSwitchOn is not printed. `posHV`/`negHV` are decoded semantic values. State and fault are enum integers; current state numbering is in [STATE_MACHINE.md](STATE_MACHINE.md). Voltage is printed without its validity flag. The serial line alone therefore cannot prove calibration quality or explain a raw/debounced mismatch.

For further race/noise investigation, proposed logging improvements are raw switch state, both event flags, STARTING phase, raw selector ADC, and timestamps close to acquisition/output application. These are suggestions, not present logging fields.

The earlier proposal to create separate MainApp and DiagnosticApp files remains optional. The current `#ifdef` selects entry-point behavior but does not imply all included libraries are absent from the other build.
