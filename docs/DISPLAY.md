# LCD and operator feedback

Status: checked in `lib/Display/Display.cpp` and `Display.h`. Physical LCD colors/legibility were not retested during documentation preparation.

## State colors

| State | Color | RGB |
|---|---|---|
| STARTUP | White | 255, 255, 255 |
| IDLE | Cyan | 0, 255, 255 |
| STARTING | White | 255, 255, 255 |
| RUNNING | Magenta | 255, 0, 255 |
| STOPPING | White | 255, 255, 255 |
| COMPLETE | Green | 0, 255, 0 |
| ABORTED | Yellow | 255, 255, 0 |
| INTERLOCK_OPEN | Blue | 0, 0, 255 |
| FAULT | Red | 255, 0, 0 |

The conversation explicitly reconfirmed these values. STARTING/STOPPING white and ABORTED yellow were additions during implementation to the original state palette.

## Screens

| State | Presentation |
|---|---|
| STARTUP | STARTUP / SELF CHECK |
| IDLE | Configured timer; rejected-start reason while switch remains ON, otherwise target estimates or calibration reminder |
| STARTING / VALIDATING | STARTING / CHECKING... |
| STARTING / WAITING_FOR_HV_ON | STARTING / WAIT FOR HV |
| RUNNING | Remaining seconds; measured voltage/current where valid, or voltage plus I:TODO |
| STOPPING | STOPPING / WAIT HV OFF |
| COMPLETE | COMPLETE / SWITCH OFF |
| ABORTED | ABORTED / INCOMPLETE RUN |
| INTERLOCK_OPEN | INTERLOCK OPEN / CLOSE + SW OFF |
| FAULT | FAULT / fault-specific text |

Each row is limited to 16 characters, padded or truncated. Longer calibration fallback text can be clipped by this limit. Color is updated on first refresh/state change; row text is written only when changed. Normal refresh scheduling is 100 ms. The implementation avoids repeated `lcd.clear()` calls.

Remaining RUNNING seconds are rounded upward from remaining milliseconds. Outside RUNNING, `remainingRunSeconds()` returns zero. Target and monitor values use kV and µA magnitudes with a separate polarity sign.

## Fault text

| FaultCode | LCD text |
|---|---|
| STARTUP_SWITCH_ON | SWITCH ON BOOT |
| INVALID_INTERNAL_STATE | INTERNAL ERROR |
| HV_PRESENT_BEFORE_START | HV ALREADY ON |
| HV_START_TIMEOUT | HV START TIMEOUT |
| HV_STOP_TIMEOUT | HV STOP TIMEOUT |
| HV_LOST_WHILE_RUNNING | HV LOST |
| WRONG_SUPPLY_ACTIVE | WRONG HVPS ON |
| INTERLOCK_OPENED_WHILE_ENABLED | DOOR OPENED |
| POLARITY_CHANGED_WHILE_ENABLED | POLARITY CHANGE |
| POLARITY_SIGNAL_INVALID_WHILE_ENABLED | POLARITY SIGNAL |
| POSITIVE_SUPPLY_FAULT | POS HVPS FAULT |
| NEGATIVE_SUPPLY_FAULT | NEG HVPS FAULT |

Start rejection text covers door open, unknown polarity, and positive/negative HV already ON. The current enum/display has no cable-detector messages.

## Presentation limitations

Active targets and voltage readback now use empirical data. Production invalidates out-of-range voltage and saturated targets. Actual current readback remains unavailable; marked I~ estimates replace it for the current workflow, and I~-- marks an unavailable estimate. These readouts are not independent protection measurements. See [CALIBRATION.md](CALIBRATION.md).

The display consumes controller state; it never grants relay permission. A green screen or an HV_OFF input is not a substitute for the laboratory's physical discharge/verification procedure.

## Updated calibration presentation

IDLE uses `I~` for voltage-pot-based expected current. At ADC 4095 it displays `TARGET SATURATED`; voltage outside calibration shows `OUT OF CAL RANGE`. RUNNING prefers measured current, otherwise shows voltage-monitor-based estimated current using `I~` (for example `+29.7kV I~22.6uA`, 16 characters). Missing/out-of-range current estimates use `I~--`; out-of-range voltage shows `V OUT OF RANGE`. These estimates assume the same setup and maximum current-limit setting.
