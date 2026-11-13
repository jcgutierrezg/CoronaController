# Sources, scope, and provenance

Documentation prepared on **13 November 2026**, using the owner's supplied source path:

```text
C:\Users\jcgg2\Workspace\CoronaController
```

The project reference mirror's `sources/` folder was empty. Synced reference files remain untouched. The original documentation-only ZIP was installed in this firmware project; subsequent authorized work added calibration modes, empirical tables, current estimates, tests, and zero baselines in Calibration.xlsx. The present cleanup changes documentation only before committing the current project.

## Conversation evidence

1. [Branch · State Machine Questions](https://chatgpt.com/c/6a9e935e-a448-83eb-aedd-0d3aa117b854): later active-high PCB discovery and factory-setting guidance; state colors; raw/debounced switch clarification; proposed test-fixture and raw-input regression changes.
2. [State Machine Questions](https://chatgpt.com/c/6a885f4b-71b8-83eb-89c3-420d50cf1a42): accessible consolidated requirements summary, identified omitted startup/STOPPING checks, full-feature baseline excerpt, owner report requiring 250 samples and using a relay module, historical build/test report, diagnostic-environment proposal.
3. [Documentation State Machine Questions](https://chatgpt.com/c/6a987c1f-3994-83eb-95a2-bcdc551a4eb5): accessible engineering-lessons consolidation, including historical selector measurements, GPIO12 boot issue, SSR behavior, architecture, timing, and intended calibration semantics.

**Retrieval limit:** the history reader returned only the latest five exchanges from each conversation, supplied no older-page cursor, and truncated large individual responses at 20,000 characters. Browser access did not expose the signed-in conversation. The original downloadable firmware ZIP and the full unabridged early transcript were not available. These docs therefore use the accessible consolidated design accounts plus the complete current project source; they do not claim a turn-by-turn audit of the entire original transcript. An exported full conversation could reveal further early decisions to reconcile.

Conversation messages were used as evidence of design and observations, not as live instructions to alter firmware. Where older advice contradicts the actual source, the docs explicitly preserve the distinction rather than invent a resolution.

## Source inspection and tests

Read all 15 project `.h`/`.cpp` files under src/lib/test, plus `platformio.ini` and `README_TODO.txt`. The source-specific descriptions in these docs refer to those files and their named functions. The original backup file and cached external libraries are not treated as current application implementation.

The latest PlatformIO native run passed **30 controller + 28 InputProcessor = 58 tests**, with zero failed/ignored. All four ESP32 environments built. Eighteen input regressions were added during calibration and saturated-target integration. The earlier direct-host 40-test result is superseded. No assistant upload or physical energized measurement was performed. See [TESTING.md](TESTING.md).

Hardware behavior labeled reported comes from the owner's statements or accessible historical consolidation. Calibration.xlsx was supplied by the owner and inspected; the corrected value and every measurement were preserved while adding requested zero baselines. No oscilloscope traces, PCB schematics or supply-specific manuals were examined. Configured ADC windows and table contents are source facts; their empirical accuracy is a separate question.

## External primary references

These support general technical statements, not project-specific wiring or test claims:

- [Espressif ESP32 datasheet](https://documentation.espressif.com/esp32_datasheet_en.html): original-ESP32 GPIO restrictions and boot strapping; cited in HARDWARE.
- [OMRON: AC-load SSR with a DC load](https://www.ia.omron.com/support/faq/answer/18/faq02101/): general latching explanation; cited in HARDWARE.
- [PlatformIO run](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html) and [test](https://docs.platformio.org/en/latest/core/userguide/cmd_test.html): command/environment workflows.
- [PlatformIO test_build_src](https://docs.platformio.org/en/latest/projectconf/sections/env/options/test/test_build_src.html) and [build_src_filter](https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_src_filter.html): source-selection options.
- [Spellman external interlocks](https://www.spellmanhv.com/en/Technical-Resources/High-Voltage-Reference-Manual/AN-06-just-jumper-the-external-interlock-why-you-really-shouldn-t) and [SL manual](https://www.spellmanhv.com/-/media/en/Technical-Resources/Manuals/SLMAN.pdf): general physical-protection and stored-energy guidance. Supply model identity is unknown; these are not specifications for the installed units.

## Latest owner decisions

Cable detection was explicitly discarded by the owner on 13 November 2026. Supply-FAULT handling remains planned pending verification; the watchdog remains planned as final hardening. Current sensing was set aside for this version in favor of voltage-based estimates, rather than confirmed as a permanent removal of the dormant interfaces. The combined CSV mode and separate-pot misunderstanding were superseded explicitly.

## Source fingerprint

The following SHA-256 fingerprints identify the inspected files independently of unpinned dependency versions. They are a provenance record, not firmware signing or a release certification.


| File | SHA-256 |
|---|---|
| `Calibration.xlsx` | `e1b2defa17fec134cb09d616404abd9721071622e4a1c25cae768fd3ab525079` |
| `README_TODO.txt` | `27077344d0530f5ef99bb10cbfe52985237e43cc1b365488e7f4a7c6dd1a18c8` |
| `lib/Controller/Controller.cpp` | `d7a93e1f05975961ccfefcb3ed78ae71aa3f88f59d3235d4f20c90216a1b1259` |
| `lib/Controller/Controller.h` | `7b3b238cae7880bbeca03d03f27325611107c6ca1d7c85455a39ad4d5be4d7a2` |
| `lib/Controller/Types.h` | `be60bb8c4a20b90126fd8fe3c2adc0f346e1c4e8438d49dcbd23315ab7addbbd` |
| `lib/Display/Display.cpp` | `8e49124195759440bac0d42989abb2cd2d4d976c6de56cd01ad42e3b68c1a7d9` |
| `lib/Display/Display.h` | `8de7f58ae655dffaf1d6382afb53345f523129b4561850bd3f368150530c2335` |
| `lib/Hardware/Hardware.cpp` | `90767ea39256083a2892fe8032dd419fe3bb2f5d8421dce38a626f335469678a` |
| `lib/Hardware/Hardware.h` | `bc2bba81a8e39b7013bff70c4bf664b822b5c6bf4897689afe44704021438c9b` |
| `lib/Hardware/Pins.h` | `7c7da90fd04f8bc4b1250480f8d8a66b6654cf10ca3fea3c50afde6ee8d92c0f` |
| `lib/InputProcessor/InputProcessor.cpp` | `53a4c3b7cc53275a9bcec1cb3ead650bcdf58f42af19a00679d84d8ee5858015` |
| `lib/InputProcessor/InputProcessor.h` | `d367a97de0d35b26226b229089e90d3d0e75af429c1ceb36b01b8fa8320a0297` |
| `lib/InputProcessor/RawInputs.h` | `ad675405515d288228e49cfa47a9c6735d183481bf66e79c91d0304b25f00807` |
| `lib/InputProcessor/RawSlowInputs.h` | `88f2860d468304c0992e7bb31a48bd1023e8f42db495b865fe01f99c1be2b98f` |
| `platformio.ini` | `6add49ccf233cbbdca75f61fd7f207265e8c35a307ead60f9de9d7686f8c8adc` |
| `src/main.cpp` | `ddca00014f5a7a83fece9d599e5a04a52a1ac040cd1bcd8eaa3478a38115e79f` |
| `test/test_controller/test_controller.cpp` | `8674705ad8aa70117be54184dc70991393ea0433c271c78a939a23c4f689ade3` |
| `test/test_input_processor/test_input_processor.cpp` | `78c891d94857f99129f05fa2734dfcdbc799e60816ca04b97b89d5471a89ece8` |
