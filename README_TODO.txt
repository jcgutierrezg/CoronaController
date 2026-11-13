CoronaController consolidated baseline
=====================================

Implemented behavior
--------------------
- STARTUP self-check; ENABLE is forced LOW first.
- Run switch ON at boot -> latched FAULT.
- Unexpected HV at startup/pre-start -> FAULT.
- IDLE / INTERLOCK_OPEN no-auto-start behavior.
- STARTING validation + WAITING_FOR_HV_ON phase.
- Timer and polarity captured at start; voltage/current pots remain live.
- Run timer begins only after the selected supply's HV_ON is confirmed.
- HV start and stop timeouts.
- Normal timer end -> STOPPING -> COMPLETE.
- Manual early stop/cancel -> STOPPING -> ABORTED (or ABORTED before energizing).
- ABORTED minimum display interval and switch-OFF acknowledgement.
- COMPLETE latches until switch OFF.
- Door/interlock opening while energized -> FAULT; never resumes automatically.
- Opposite polarity while energized -> immediate FAULT.
- Brief UNKNOWN polarity tolerated; sustained UNKNOWN -> FAULT.
- Wrong supply active -> FAULT.
- Active HV disappearing during RUNNING -> FAULT.
- HVPS FAULT support present behind a config flag.
- STOPPING still applies energized safety checks until actual HV_OFF.
- Fault reset requires post-fault run-switch interaction, final switch OFF, and both supplies HV_OFF.
- If the fault occurred while switch was already OFF, this naturally requires OFF->ON->OFF.
- Output is whitelist-derived: only STARTING/WAITING_FOR_HV_ON and RUNNING enable HV.
- 250-sample fast polarity ADC averaging (required by current breadboard relay transient behavior).
- Local per-channel averaging; no shared cross-channel ADC buffer.
- Measured voltage empirical lookup/interpolation for both polarities through 15.3 kV.
- Lookup-table framework for measured current and empirical target estimates.
- 16x2 RGB LCD state/status display with write-on-change behavior.

TODOs requiring hardware data / later hardening
-----------------------------------------------

1. HVPS FAULT inputs
   - Characterize exact semantics/electrical behavior.
   - Set HardwareFeatures::NEGATIVE_FAULT_WIRED = true.
   - Set InputCalibration::useFaultInputs = true.
   - Set ControllerConfig::useSupplyFaultInputs = true.

2. Current measurement
   - Measure positive and negative current-monitor ADC/uA points.
   - Populate both monitor tables in src/main.cpp.

3. Target display QoL
   - Correct physical pot wiring so CCW=0 and CW=max.
   - Empirically measure actual expected loaded output vs pot ADC.
   - Populate POS/NEG voltage and current target tables.

4. Voltage measurement
   - Add calibration points above 15.3 kV if required.
   - Current implementation deliberately clamps above the last measured point.

5. Timeouts
   - Measure real HVPS startup/shutdown times.
   - Tune HV_START_TIMEOUT_MS and HV_STOP_TIMEOUT_MS.

6. ADC / EMC
   - FAST_ADC_SAMPLES=250 is based on the current breadboard behavior.
   - Improve relay supply isolation, grounding, decoupling and final PCB layout, then see whether this can be reduced.

7. Watchdog
   - Add ESP32 task watchdog after normal functional tests are stable.
   - Watchdog is supplementary; it does not replace the physical safety chain or feedback checks.
