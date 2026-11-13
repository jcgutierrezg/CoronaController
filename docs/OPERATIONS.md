# Operating behavior

This describes the firmware's operator interface after approved hardware commissioning. It is not an isolation, discharge, access, or energized-testing procedure. Follow the laboratory procedure and [SAFETY.md](SAFETY.md).

## Ready and start

With the maintained switch OFF, startup checks for an ON switch or pre-existing HV. A closed interlock leads to cyan IDLE; an open interlock leads to blue INTERLOCK_OPEN. Starting with the switch already ON latches FAULT rather than treating power-up as a start request.

In IDLE, the timer pot sets 5–120 seconds. The displayed target voltage/current is presently an endpoint estimate awaiting empirical calibration. Validated operation requires a known selector polarity and no pre-existing HV; cable verification is not currently implemented.

A new debounced ON event requests STARTING. During validation enable is OFF. After acceptance the screen says WAIT FOR HV and the controller commands enable. RUNNING and the treatment timer begin only when the selected supply reports HV_ON. Failure to confirm within the current 500 ms timeout faults.

## During treatment

RUNNING is magenta and shows remaining time. Duration and polarity have been captured; adjusting the timer pot does not change this treatment. Voltage/current pots remain live. Measured voltage has calibration only through 15.3 kV, and measured-current calibration is still TODO.

Normal time expiry commands OFF and enters white STOPPING. The firmware waits for the selected supply's HV_OFF report, then shows green COMPLETE. Returning the switch OFF acknowledges completion and returns to IDLE or INTERLOCK_OPEN.

## Cancellation

Physical switch OFF is acted upon on the next fast processing cycle once enable has been commanded, without waiting for stable OFF debounce. The result goes through STOPPING to yellow ABORTED, not COMPLETE. Cancellation during VALIDATING can go directly to ABORTED because enable has not yet been commanded.

ABORTED means incomplete treatment. It remains visible for at least 2.5 seconds and requires stable switch OFF before leaving. STOPPING failure to obtain HV_OFF instead becomes a latched fault.

## Interlock and fault acknowledgement

Opening the interlock during energized operation invalidates the run. Closing it does not resume the treatment. In INTERLOCK_OPEN, door closure plus switch OFF is required before readiness; a fresh ON request is needed for another run.

FAULT is red and disables the command output. After resolving the cause according to the approved procedure, acknowledgement requires post-fault switch interaction, final stable OFF, and both supplies reporting HV_OFF. If the switch was already OFF when FAULT occurred, remaining OFF is insufficient; the software requires OFF → ON → OFF. ON while still in FAULT is an acknowledgement interaction, not a run command.

The current code's reset and feedback limitations are recorded in [SAFETY.md](SAFETY.md). Neither COMPLETE, ABORTED, FAULT, nor an HV_OFF report independently establishes safe access to hardware.
