# Implementation status and evidence

Snapshot: 13 November 2026. “Source” means inspected local files; “native” means executed host tests, not hardware validation. [SOURCES.md](SOURCES.md) describes the available conversation history.

| Feature / requirement | Current implementation | Evidence / remaining work |
|---|---|---|
| PlatformIO ESP32 + native testing | Present | `platformio.ini`; native suites rerun directly with host compiler |
| Layered modules | Present | Hardware / InputProcessor / Controller / Display |
| Nine states and two STARTING phases | Present | Source; selected native scenarios pass |
| Startup switch-ON and unexpected-HV faults | Present | Source + tests |
| No automatic interlock resume | Present | Source + tests |
| Captured timer/polarity; timer begins at HV confirmation | Present | Source + tests |
| Voltage/current controls remain live | Not captured by controller | Source; physical wiring behavior not remeasured |
| HV start/stop timeout | Present, 500 ms each | Source + tests; values still need measured tuning |
| COMPLETE versus ABORTED | Present | Source + tests; ABORTED minimum 2500 ms |
| Latched fault/reset interaction and both-HV-OFF reset | Present | Source + tests |
| Output allow-list | Present | Source; existing test only samples safe-state coverage |
| Energized interlock / polarity / wrong-HV checks | Present | Source + selected tests; concurrent STARTING and STOPPING gaps remain |
| UNKNOWN polarity grace | Present, 100 ms | Source + tests; hardware tuning pending |
| Raw/debounced switch race fix | Present | Two controller regression tests pass; raw InputProcessor regression still missing |
| HV_ON active-high after PCB assembly | Configured in factory | Owner-reported hardware finding + source assignment; old comment stale |
| FAULT active-high after PCB assembly | Polarity configured | Owner-reported finding; decoding/response still disabled |
| Negative FAULT signal read | GPIO16 always read | Source; physical connection/semantics need verification |
| `NEGATIVE_FAULT_WIRED` feature flag | Absent | Older advice and README_TODO reference do not match current code |
| Cable detector | Absent from current code | Previously discussed; no pin/types/config/checks; removal intent unresolved |
| Current pot moved away from GPIO12 | GPIO34 assigned | Source + historical boot-failure account |
| Fast ADC local averaging | 250 samples | Source + owner report that 8 was insufficient; latency/PCB retest pending |
| Slow ADC local averaging | 100 samples/channel | Source |
| Current PCB selector windows | Configured | Open 0–200, POS 800–1600, NEG 1900–2650; measurement record absent |
| Positive/negative voltage-monitor lookup | Present through 15.3 kV | Source labels recorded calibration; not independently remeasured |
| Current-monitor lookup | Empty | TODO; values invalid for display |
| Four target-pot lookups | Assigned endpoint placeholders | Numeric target-valid flags can be true; empirical calibration incomplete |
| Correct CCW/CW pot wiring | Intended convention | Still needs physical confirmation |
| Fixed-width RGB LCD | Present | Source; no physical display test performed |
| Diagnostic environment | Present | Single-file conditional branch; commands disabled output each loop |
| MainApp/DiagnosticApp split | Not present | Earlier architectural suggestion, optional |
| Task watchdog | Not implemented | Explicit deferred-hardening TODO |
| Continuous unexpected-HV standby guard | Not present | New source-review finding |
| Both supplies OFF before STOPPING result | Not enforced | New source-review finding |
| Fault deassertion required for reset/start | Not enforced | New source-review finding |

## Avoid these misleading descriptions

- “Everything discussed is implemented.” Cable detection is absent, watchdog is deferred, and calibration is incomplete.
- “The fault inputs are now active and protecting the run.” Their polarity is configured active-high, but both processing/response switches are false.
- “The target tables are missing.” They are assigned and active, but only placeholder endpoints are compiled.
- “All current hardware has been verified.” The source, existing tests, and owner observations are different kinds of evidence.
- “All tests passed, so the PCB is validated.” Forty existing host tests passed; their synthetic calibration is not the production PCB configuration.
- “STOPPING or FAULT means discharged.” These are software states; feedback interpretation and stored-energy safety require separate hardware procedures.

No firmware fixes were made while producing these docs. The unresolved requirements and source-review findings remain visible in [TODO.md](TODO.md).
