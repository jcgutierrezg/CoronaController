# Implementation status and evidence

Reviewed 13 November 2026. Source presence, owner-reported hardware behavior, passing software tests and pending physical verification are separate evidence categories.

| Feature | Current status | Evidence / remaining limits |
|---|---|---|
| Layered firmware; nine states; two STARTING phases | Implemented | Source + native tests |
| Startup switch/HV checks, no interlock resume, COMPLETE/ABORTED, latched reset | Implemented | Controller tests; edge-case reviews R01–R04 still open |
| Timer/polarity captured; time begins at HV confirmation | Implemented | Source + tests |
| Raw OFF before RUNNING HV-loss detection | Implemented | Two controller race tests; input-layer raw-OFF regression still missing |
| HV_ON / FAULT active-high | Configured | Owner-reported PCB discovery; HV_ON source comment corrected |
| Supply-FAULT handling | Disabled, retained pending verification | Both decode/response options false; GPIO39/16 are acquired |
| Cable detection | **Discarded by owner** | Absent code reflects current scope; not a pending implementation |
| Task watchdog | Planned, not implemented | Retained explicitly |
| Voltage target/monitor data | Integrated | Corrected workbook, separate polarities, through 29.7 kV monitor range |
| Zero baselines | Added by owner request | All four workbook blocks and every firmware table; distinguish from additional physical measurements |
| Pot ADC 4095 handling | Implemented | Both observations retained in Excel; targets marked ~ at 4095 (live monitor or calibrated midpoint); monitor-based RUNNING readback remains distinguishable |
| Expected IDLE current | Implemented estimate | Voltage-pot ADC, selected polarity, I~ label |
| RUNNING current | Implemented estimate | Active supply's voltage-monitor ADC; I~ label; assumes same geometry/load and current limit at maximum |
| Actual current feedback | Unavailable; set aside for current setup | ADC_0db trial still read zero on both; arrays contain one zero baseline each; no hardware cause independently proved |
| Preference for future measured current | Implemented | LCD prefers valid measured-current fields over estimates |
| Voltage/current-estimate range flags | Implemented | Production refuses out-of-range values; estimates not used by safety/controller decisions |
| Pin correction | Applied by owner | Positive current GPIO26; negative voltage GPIO25; positive voltage GPIO27; negative current GPIO33 |
| Shared target pots | Correct model | One voltage pot, one current pot; not separate pots per supply |
| Fast/slow averaging | Implemented | 250 selector / 100 per slow channel; PCB latency/noise verification remains |
| Firmware environments | Four present and built | normal, diagnostic, calibration_voltage, calibration_current |
| Host tests | 58 pass | 30 controller + 28 InputProcessor; not bench validation |
| Optional app-file split/version pinning | Not required for current functionality | Optional maintenance/release improvements |

Remaining work is centralized in [TODO.md](TODO.md). Direct current readback was replaced for this workflow, not silently relabeled as a real measurement. Full hardware commissioning and R01–R04 remain open despite successful builds/tests.
