CoronaController status
======================

The maintained backlog is docs/TODO.md.
The current implementation matrix is docs/IMPLEMENTATION_STATUS.md.
Calibration values and assumptions are documented in docs/CALIBRATION.md.

Updated 13 November 2026:
- Cable detection explicitly discarded.
- Supply-FAULT handling retained, pending verification (currently disabled).
- Watchdog retained as planned final hardening.
- Direct current readback set aside for this setup; voltage-based current estimates integrated.
- Voltage calibration through 29.7 kV, zero baselines and 4095 saturation handling integrated.
- Four firmware builds and 58 native tests passed.

Do not use old instructions referring to NEGATIVE_FAULT_WIRED, missing calibration arrays,
the 15.3 kV-only tables, or a combined calibration CSV environment.
