# SMK-37 physical test results

Public logs use stable unit aliases instead of unique MAC addresses; exact identifiers remain in an untracked local record.

Status: Cardputer 1.0 universal compatibility reported working by the user. SMK-37 Bluetooth startup remains faulty: the user reports needing two keyboard off/on cycles before key input works. A fix is planned for a coming version. Full per-revision acceptance remains pending.

## M5Chord 1.0 release preparation — 2026-09-13

- User result: after the universal compatibility upload documented below, the user reported `working` on the connected Cardputer 1.0. This is a basic user smoke result, not acceptance of every mode, key, transport or persistence workflow. Power/cable/controller/receiver details beyond the preceding programming setup were not supplied.
- SMK-37 result: the user reports the Bluetooth keyboard needs to be switched off and back on twice before keys register. Exact keyboard firmware, connection order and traffic were not captured. The startup issue remains open and is documented in README/release notes for a coming-version fix, without claiming a root cause.
- Release changes: M5Chord 1.0 branding on boot, BLE local identity and diagnostics; settings namespaces and SD paths unchanged. Builds/tests and packaging are performed without reflashing a device in this release-preparation session.
- Release application SHA-256: `feff17dc0d62c565991c7a6bf502e56c91191dea8b115ff9607eee472524bfb3`. Both PlatformIO builds, strict CTest and ASan/UBSan CTest passed. Esptool reports a valid image checksum and validation hash. Firmware startup frame was rendered with the actual M5GFX renderer for inspection; this is not an LCD observation.
- Release-specific physical acceptance: pending. The user's `working` report applies to the preceding compatibility application hash, not automatically to the newly renamed release binary. Classic 1.1 remains untested; older ADV results remain scoped to their recorded builds.

## Universal 1.0 / 1.1 / ADV compatibility — 2026-09-13

- Request: build one compatibility candidate for all three revisions. The user subsequently reported a Cardputer 1.0 connected in bootloader for the ongoing build/upload workflow.
- Physical programming setup: user-identified Cardputer 1.0 connected to the Mac by USB at `/dev/cu.usbmodem12101`. Esptool identified ESP32-S3 revision 0.2, unit ID `CLASSIC-TEST-01`, flash manufacturer `c8`, device `4017`, 8 MB quad flash. This differs from the earlier ADV MAC. Product revision identification is from the user, not esptool. Cable model, battery state, SD presence, attached controller and DIN receiver were not observed.
- Changes: common pins; removal of SD GPIO5 writes; library-selected classic matrix versus ADV TCA8418 keyboard; unsupported-board startup guard; Geek family label; snapshot-based polling with same-count swap/release regressions. No musical policy, settings schema, key binding or font changes.
- Validation: strict CMake/CTest and ASan/UBSan CTest passed. Fresh `cardputer-universal` and legacy `cardputer-adv` PlatformIO builds passed. Nine package regression tests passed. Universal linker usage: 106,372 bytes static RAM (32.5%), 1,326,413 bytes flash (39.7%); not runtime heap/stack measurements.
- Universal application: `.pio/build/cardputer-universal/firmware.bin`, 1,326,784 bytes; SHA-256 `75b6ff26bb168b7054c6b4093d5cb103c9cd0201265cd6e640ddc551bc2a9dfc`.
- Upload command: `PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-universal -t upload --upload-port /dev/cu.usbmodem12101`.
- Programming result: succeeded at 1,500,000 baud. Bootloader at zero, partition table at `0x8000`, boot application at `0xE000`, and firmware at `0x10000` each passed flash hash verification. The uploader issued a hard reset. Prior firmware in these programming regions was replaced; no whole-chip erase or SD modification was performed. NVS was outside erased/written regions; runtime persistence remains unverified.
- After upload: the Mac no longer listed the programming port. Firmware uses native USB as a host, so disappearance alone establishes neither successful startup nor a fault. No screen, keyboard, BLE, USB-host or DIN result was observed by the agent.
- Factory package: `dist/cardputer-universal-compat-2026-09-13/cardputer-universal-factory.bin`, 1,392,320 bytes; SHA-256 `56d9c134a3b0052f10ca9325a0b8c8a6c1b88a4ce6499bfb7ed910305fbca836`. Input and merged segment bytes verified; factory NVS verified erased. This settings-resetting image was not used for the 1.0 upload. No flash dump or M5Burner publication was performed.
- Acceptance: 1.0 programming verified only; 1.0 runtime and all 1.1/ADV checks for this candidate remain pending. Follow the universal checklist in `TEST_PLAN.md`, especially keyboard/SD coexistence, first BLE input, modifier releases, stable display and MIDI panic. Earlier ADV observations do not accept this candidate.

## Retro instrument interface and live output keyboard — 2026-09-13

- User feedback on the prior image: text too tight, disliked font, insufficiently dominant played chord, unwanted performance instructions. Reference `IMG_2280.HEIC (user-supplied reference, not distributed)` was inspected via a format-converted PNG; the original was not modified. Its visible setup shows the Cardputer display, KEY, C Hijaz 12T, Free, Pattern 100, toggled m7 and transport indicators. The photo does not establish keyboard/receiver connections, timing or audio results.
- Direction: loosely late-1970s shipboard instrumentation, black/phosphor green/amber, local Orbitron display lettering and terminal labels, large played harmony, a single spaced modifier rail, no instructional performance prose, and a quieter Tab Options layout. Design rationale is in `RETRO_DISPLAY_DESIGN.md`.
- Live keyboard: accepted post-scheduler/post-registry output messages feed a portable bounded gate bitmap and 64-entry attack ring. No allocation, UI rendering, storage access or altered MIDI deadlines in the observation path. Green represents an output Note On gate; a 50 ms amber mark represents a recent attack. KEY/CHORD select the performance output channel; BYPASS includes all channels. Loop output is included when routed to the displayed channel. Keyboard UI requests up to 60 Hz updates, other views 30 Hz. These are software targets, not measured physical frame rates or wire latency.
- Verification: strict CMake build and CTest passed; ASan/UBSan CTest passed; pinned `pio run -e cardputer-adv` passed. Added exact arp/strum timing tests, all eight performance modes in four directions, one-percent gates, overlap, loops, channel isolation, late dispatch, failed sends, MIDI bounds, panic/disconnect and lane-change cleanup. No comments or `delay()` calls found in `src`/`test`.
- Visual QA: actual 240×135 M5GFX frames for all 51 Options rows, five editor fields, learning/idle/held/stacked/long-chord views and eight 50-frame output sequences. Checked chord hierarchy, pad spacing, green/amber states, long sharp-root names, menu value fitting, footer clipping and repeated-frame stability. The preview script asserts that static header/modifier regions stay pixel-identical during sequences. Initial apparent label loss in displayed contact sheets was not reproduced in the framebuffer pixel comparisons; no hardware rendering failure is inferred from it.
- Artifacts: `build-preview/retro/performance.png`, `menus.png`, `live-arp.gif`, `live-strum.gif` and corresponding sequence/timeline files. The 8-bit framebuffer remains 32,400 pixel bytes. Native preview artifacts do not establish physical LCD readability.
- Candidate SHA-256: `ed0c4db5fa013872e8e3462f31e619a476190f3ac8bcac30822121d5ef3f7d6b`; firmware path `.pio/build/cardputer-adv/firmware.bin`. Linker usage: 106,372 bytes static RAM (32.5%), 1,326,357 bytes flash (39.7%). Runtime heap/stack margins have not been measured.
- Authorization: user reported the Cardputer attached during this requested redesign; proceeding with the established build-and-flash workflow. Programming port `/dev/cu.usbmodem12101` is visible. Exact cable, SD presence, keyboard and DIN receiver state are not observed during programming.
- Upload command: `PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-adv -t upload --upload-port /dev/cu.usbmodem12101`.
- Programming result: successful at 1,500,000 baud. ESP32-S3 revision 0.2, unit ID `ADV-TEST-01`, matches the earlier Cardputer. Bootloader, partition table, boot application and firmware all passed flash hash verification; the uploader issued a hard reset. Programming regions replaced the previous firmware. No whole-chip erase or SD modification was requested; NVS persistence after reboot remains a physical check.
- Hardware acceptance: pending. Check this revision's screen at normal playing distance, first-connection SMK-37 input, visible arp/strum gates, DIN note cleanup and saved settings using the retro/live-keyboard checklist in `TEST_PLAN.md`.

## KEY extensions and interface revision — 2026-09-13

- Request: address independent KEY extensions and UI/UX, use a consistent font; user reported the Cardputer connected for the planned upload.
- Changes: scale-derived base triad plus literal 6/m7/M7/9 additions; independent seventh release; safe low/high MIDI-boundary folding. The display separates mode, scale application, quality/playstyle, held/selected pads, and extension HOLD/TOGGLE. All screens use bundled DejaVu Sans; Options adds neighboring rows, contextual help, and ON/OFF values. Longer mapping descriptions wrap. No new bindings, code comments, or blocking timing calls.
- Validation: strict CMake build/CTest and ASan/UBSan CTest passed. Tests cover all 12 scales, all MIDI roots and extension masks, exact intervals, common-tone preservation, independent releases, stacking, overlapping roots, timed-mode cleanup, display states, and menu presentation. The pinned PlatformIO target build passed.
- Visual QA: the actual M5GFX renderer and font bitmaps produced 240×135 previews of performance modes/views, all 51 Options rows, mapping-editor fields, learning, and feedback. Corrected shortcut/status/footer clipping and adapted colors to the existing 8-bit framebuffer; its 32,400-byte pixel allocation is unchanged. This is native rendering evidence, not physical LCD acceptance.
- Firmware: `.pio/build/cardputer-adv/firmware.bin`; SHA-256 `8285a83d85ff2a6051746ee1d5b51259d60d5fafa3d4579aad7ca6a46dda2e0e`. Static RAM 105,044 bytes (32.1%); flash 1,321,037 bytes (39.5%). Linker figures do not establish runtime heap/stack margin.
- Programming setup: Cardputer ADV connected to the Mac over USB at `/dev/cu.usbmodem12101`; uploader identifies ESP32-S3 revision 0.2, unit ID `ADV-TEST-01`, matching earlier uploads. Cable model, SD presence, SMK-37 startup order/connection, DIN receiver, and audio output were not observed.
- Upload command: `PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-adv -t upload --upload-port /dev/cu.usbmodem12101`.
- Programming result: upload succeeded at 1,500,000 baud; bootloader, partition table, boot application, and firmware all passed flash hash verification. The uploader issued a hard reset. Only programming regions were replaced; no whole-chip erase or SD modification was requested. The prior firmware was replaced and can be rebuilt from its corresponding source archive if retained.
- Physical acceptance: not performed. Follow the KEY extensions/interface checklist in `docs/TEST_PLAN.md`, including first-connection input, note cleanup, display readability, and saved settings after reboot.

## Arabic scale expansion — 2026-09-12, build only

- Request: add Arabic scale options. Added Hijaz, Hijazkar, Kurd, and Nikriz as explicitly labelled `12T` equal-tempered pitch sets. These are not quarter-tone tuning or full maqam-performance implementations; references, intervals, and usage are recorded in the README.
- Existing scale IDs 0–7 are unchanged; new IDs 8–11 are appended. Scale-menu limits, forward/reverse semantic actions, and current settings/preset decoding share the catalog count. The historical schema-1 validator retains its original eight-scale range. No key bindings, tuning messages, or note-routing paths changed.
- Validation: CMake/CTest, AddressSanitizer/UndefinedBehaviorSanitizer CTest, and `pio run -e cardputer-adv` passed. Tests check exact pitch sets, membership across every MIDI pitch and all twelve key roots, scale-derived notes and chord quantization, invalid enum fallback, menu wrap, old/new settings round-trips, invalid saved scale rejection, display labels, held-note scale changes, arp output membership, and release cleanup.
- Image: `.pio/build/cardputer-adv/firmware.bin`; SHA-256 `f25c51b455cedd96a4ca00533dd28d63571c9606eda04aa7a5d587de022b9cf0`. Static RAM 104,908 bytes (32.0%); flash 1,306,785 bytes (39.1%). Linker figures only.
- Flash authorization/setup: user reported the Cardputer connected and requested `flash`. The Mac exposed `/dev/cu.usbmodem12101`; uploader identified ESP32-S3 revision 0.2, unit ID `ADV-TEST-01`. Cable model, SD presence, SMK-37 connection, selected mode/scale, receiver, and audio result were not observed.
- Upload command: `PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-adv -t upload --upload-port /dev/cu.usbmodem12101`.
- Programming result: successful at 1,500,000 baud. Bootloader, partition table, boot application, and firmware passed flash hash verification, and the uploader issued a hard reset. Only programming regions were written; no whole-chip erase or SD modification was requested.
- Acceptance: programming verified only. New scale selection, sound, display readability, reboot persistence, and receiver behavior require the Arabic-scale checklist in `docs/TEST_PLAN.md`.

## BLE startup subscription revision — 2026-09-12

- User report: the SMK-37 appears connected initially, but keys do not register until the keyboard is turned off and back on. Exact startup order, keyboard firmware, and BLE traffic were not captured, so the physical root cause is not established.
- Source finding: the previous implementation marked the device connected unconditionally after the pinned Arduino BLE `registerForNotify()` call. That wrapper returns no subscription status and does not propagate descriptor-write failures. The reused client could also retain previously discovered services. The new sequence follows the separate registration and descriptor-write completion events documented in the [Espressif IDF 4.4 GATT client API](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/bluetooth/esp_gattc.html).
- Changes: fresh discovery per connection; 500 ms settling; asynchronous local registration followed by acknowledged notification enable; up to three rejected-write attempts; two-second operation timeouts with disconnect/re-scan recovery; one delayed enable refresh if the connection remains silent. No perpetual no-data reconnect watchdog. Green/`ON` now requires subscription acknowledgement; setup is yellow/`SUB`. Subscription attempts/status are included in diagnostic exports. Recovery gates input and invokes panic for a previously ready connection.
- Includes the display-feedback revision below. No control bindings changed, no code comments added, and no `delay()` introduced.
- Software validation: CMake/CTest, AddressSanitizer/UndefinedBehaviorSanitizer CTest, and the pinned PlatformIO target build passed. Portable tests cover settling, acknowledgement ordering, rejected writes, retry limits, registration and write timeouts, reset/late acknowledgements, bounded silent refresh, idle stability, and refresh failure.
- Candidate image: `.pio/build/cardputer-adv/firmware.bin`, SHA-256 `a4dde53204c0f6436b55dea1ed457639788ce1015ca9badc01e2719094f19069`; static RAM 104,908 bytes (32.0%), flash 1,306,597 bytes (39.1%). These are linker figures, not runtime measurements.
- Authorization/setup: user explicitly reported Cardputer in bootloader and requested flashing when done. Cardputer ADV connected to the Mac over USB at `/dev/cu.usbmodem12101`; uploader identified ESP32-S3 revision 0.2, unit ID `ADV-TEST-01`, matching previous uploads. Cable model, SD presence, keyboard power/startup order, and DIN equipment were not observed during programming.
- Upload command: `PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-adv -t upload --upload-port /dev/cu.usbmodem12101`.
- Programming result: successful at 1,500,000 baud. Bootloader, partition table, boot application, and firmware passed flash hash verification; uploader issued a hard reset. Only programming regions were written, with no whole-chip erase or SD modification requested. The previously flashed firmware was replaced; source remains available for rebuilding.
- Acceptance: programming verified only. No keyboard/DIN acceptance test has been performed for this revision. Verify keys on the first connection without restarting the SMK-37, then follow the BLE cold-start/reconnect and display-feedback checklists in `docs/TEST_PLAN.md`.

## Display-feedback revision — 2026-09-12, not flashed

- Request: build immediate modifier-pad feedback and distinguish idle key/scale from played chords. This turn did not authorize another flash; no upload, serial reset, or device interaction was performed.
- Physical setup/result: no setup exercised for this revision. The prior expansion upload below remains the last recorded programmed image.
- Changes: small labelled key/scale with inactive marker; large `READY` before playing; cached played-chord naming with a 1.2-second release linger and final 0.5-second fade; persistent green held/blue selected-or-stacked pad rows across performance views and parameter overlays; stale history cleared by panic. No MIDI key bindings changed.
- Software validation: CMake build and CTest passed, including new portable display-state tests for idle Key mode, pre-note pad feedback, multi-source releases, stack toggles, fade deadlines, short taps, panic, loop-only display, and held roots across gaps. The pinned `pio run -e cardputer-adv` build passed.
- Image: `.pio/build/cardputer-adv/firmware.bin`; SHA-256 `aa35bd8090d48951b34e86030a10b9ae79815e7891f784d007dc42b158c9bc27`.
- Build usage: 104,852 bytes static RAM (32.0%) and 1,306,653 bytes flash (39.1%). These are linker figures, not measured runtime headroom.
- Additional validation: AddressSanitizer/UndefinedBehaviorSanitizer build and CTest passed; no code comments or `delay()` calls were found in `src` or `test`.
- Physical display readability, timing, and musical operation remain unverified for this revision; follow the display-feedback checklist in `docs/TEST_PLAN.md` after upload authorization.

## Controller expansion upload — 2026-09-12

- Authorization: the user subsequently requested `flash`, lifting the earlier hold for this upload.
- Programming setup: Cardputer ADV connected to the Mac by USB at `/dev/cu.usbmodem12101`; ESP32-S3 revision 0.2, unit ID `ADV-TEST-01`, matching the previously programmed device. Cable model, SD presence, battery state, and connected MIDI equipment were not observed.
- Command: `PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-adv -t upload --upload-port /dev/cu.usbmodem12101`.
- Image SHA-256: `13740895a327dd5f54041adf7ce635d5cef66651f49fb6fe6dfe09d41ee75f18`, matching the verified expansion candidate.
- Result: upload succeeded at 1,500,000 baud; bootloader, partition table, boot application, and firmware all passed flash hash verification. The uploader issued a hard reset.
- Only application/programming regions were written; no whole-chip erase or SD modification was requested. Device-side settings migration/persistence remains unverified.
- Acceptance: programming verified only. Stable startup, the new menu, momentary/stacked pads, looping, MIDI output, and reboot persistence still require physical confirmation.

## Controller expansion — 2026-09-12, build only

- Authorization: the user explicitly requested holding off on flashing. No upload command, serial reset, or physical device test was performed in this expansion session.
- Physical setup/result: none exercised for this candidate. Existing SMK-37 BLE and EasyPlay USB observations below describe older firmware only.
- Software: expanded Options and conflict-checked key dispatch; release-gated learning; general mapping editor and profile matching; Simple/Advanced/Free/Latched; independent stream routing; velocity/transpose; 16 preset slots; bounded MIDI record/play/overdub/undo/quantize/save/load; deferred storage and diagnostic-ring export; DIN/BLE overload recovery.
- Validation: CMake build and CTest passed with warnings as errors. AddressSanitizer/UndefinedBehaviorSanitizer build and CTest passed. `PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-adv` passed.
- Final build usage: 104,652 bytes static RAM of 327,680 (31.9%); 1,304,977 bytes flash of 3,342,336 (39.0%). These are linker figures, not measured runtime heap/stack margins.
- Candidate: `.pio/build/cardputer-adv/firmware.bin`.
- SHA-256: `13740895a327dd5f54041adf7ce635d5cef66651f49fb6fe6dfe09d41ee75f18`.
- Code audit: no code comments or `delay()` calls found in `src` or `test`.
- Remaining physical gates: keyboard rollover/conflicts on the device, all learn/edit workflows, SD save/reboot/interrupted-write behavior, note/loop/sustain cleanup, BLE overflow/reconnect, direct SMK USB investigation, measured DIN throughput/latency and endurance. Follow `docs/TEST_PLAN.md` only after flashing is authorized.
- Controller comparison: `docs/ORC1_CONTROLLER_ASSESSMENT.md`; synth/audio features are excluded, and remaining controller differences are listed explicitly.

## Startup investigation — 2026-09-12

- Programming setup: Cardputer ADV connected to the Mac through USB at `/dev/cu.usbmodem212101`; ESP32-S3 revision 0.2, unit ID `ADV-TEST-01`. Cable model, attached MIDI equipment, SD presence, and battery state were not recorded.
- Initial upload: flash hash verification passed. This verified programming only, not application startup or MIDI operation.
- Physical result reported by the user: switching on makes the screen flicker on and off, with no further visible activity. No crash log was captured; a reset loop is suspected but unconfirmed.
- Source finding: positional initialization of `usb_host_config_t` assigned the interrupt flag to `skip_phy_setup`, leaving USB PHY setup disabled. The patch initializes both fields explicitly.
- Additional startup changes: disabled unused internal audio and IMU initialization, added visible startup stages, rendered UI through an offscreen framebuffer, and allowed the idle task to run between loop iterations. Removed a duplicate DIN polling call.
- Software verification: CMake build and CTest passed; `pio run -e cardputer-adv` passed using the repository-local PlatformIO core directory. Reported firmware usage: 622,653 bytes flash and 57,528 bytes static RAM.
- Candidate image: `.pio/build/cardputer-adv/firmware.bin`; SHA-256 `ea93588a480133e05dd08abb6d9a86c56d5619f92e8516147ec4593fa79aa26e`.
- Reflash status: completed on `/dev/cu.usbmodem212101` at 1,500,000 baud. The ESP32-S3 reported unit ID `ADV-TEST-01`; every written flash segment passed hash verification. Application startup and MIDI behavior are still awaiting physical confirmation.
- Required physical follow-up: record connected equipment and power source, flash the candidate, power-cycle, and confirm a stable performance screen and working keyboard input. If startup stalls, record the last visible stage. MIDI acceptance tests below remain unexecuted.
- USB observations reported on 2026-09-12: the SMK-37 Pro enumerated on macOS as `SMK-37 Pro Midi`, VID `4353`, PID `CF4D`, full-speed 12 Mbit/s, and was claimed by MIDIServer. The Cardputer showed zero VID and PID for the SMK-37. An EasyPlay1 Plus enumerated directly after being placed in its MIDI mode. The EasyPlay did not enumerate through a powered UGREEN USB hub, consistent with the pinned ESP-IDF 4.4 stack lacking external-hub support. Cable, adapter, hub model, and SMK firmware version remain unrecorded.
- BLE MIDI candidate build on 2026-09-12: added automatic discovery of the standard BLE-MIDI service and the `SMK-37 Pro_BLE` identity, fixed-capacity notification-to-main-loop event queuing, reconnect scanning, disconnect panic, and Geek-view diagnostics. CMake, CTest, and `pio run -e cardputer-adv` passed. Reported usage is 1,248,397 bytes flash and 84,116 bytes static RAM. Candidate SHA-256 is `4dc6bd6691157ffa3504e587ec3eeb78eaa7f1e6cebf5e4541114b7b36f6c73b`. Physical BLE discovery, input, disconnect, and reconnect results remain pending and are not accepted.

- BLE image flash: completed on `/dev/cu.usbmodem212101` at 1,500,000 baud. The ESP32-S3 revision 0.2 reported unit ID `ADV-TEST-01`; bootloader, partition table, boot application, and firmware writes all passed hash verification. The uploader issued a hard reset. This verifies programming only; BLE hardware acceptance remains pending.
- BLE smoke result reported by the user on 2026-09-12: the flashed Cardputer connected to the SMK-37 over BLE MIDI and MIDI input worked. The exact power source, SMK-37 firmware revision, range, latency, disconnect behavior, and reconnect behavior were not recorded. BLE discovery and basic input pass by user report; broader hardware acceptance remains pending.
- Follow-up issue reported by the user on 2026-09-12: pad learning was sensitive enough for one physical press to be captured more than once, and DIN output reached only one Ambient Zero layer. The receiver firmware defaults layers 1–4 to MIDI channels 1–4. The candidate fix adds exact duplicate rejection plus a 350 ms capture guard, a ninth learned `output.lane.next` pad, the `L` shortcut, persisted primary-channel selection, visible layer state, and panic before channel changes. Physical confirmation remains pending.
- Follow-up candidate verification: CMake, CTest, and `pio run -e cardputer-adv` passed. Firmware SHA-256 is `0f6e4ef78c72ee935b4c6564142dda7f5318a3d03b1c0c323d13128b29e37b8e`. The Cardputer programming port was not visible to macOS, so this candidate was not flashed and no hardware result is claimed.

- Follow-up candidate flash: completed on `/dev/cu.usbmodem12101` at 1,500,000 baud. ESP32-S3 revision 0.2 and unit ID `ADV-TEST-01` matched the prior device. All flash regions passed hash verification and the uploader issued a hard reset. Pad-learning and Ambient Zero layer-selection behavior remain pending physical confirmation.
- Extension interaction revision requested on 2026-09-12: make learned extensions momentary by default with a Cardputer-controlled stacking mode. The candidate adds the `X` stack toggle, `MOM`/`STACK` display state, press/release handling for note and CC pads, extension clearing during mode changes, and persistence only for stacked extensions. Software verification and physical testing are pending.
- Extension interaction candidate verification: CMake, CTest, and `pio run -e cardputer-adv` passed. Reported usage is 1,249,153 bytes flash and 84,156 bytes static RAM. Firmware SHA-256 is `499d984396fdb4fd58a41bba7987eff284eb44f01fcf93e958d8482edc471303`. The programming port was no longer visible after the build, so this revision was not flashed and hardware behavior remains unverified.

## Setup

- Cardputer ADV serial:
- Firmware commit or archive:
- SMK-37 model and firmware:
- USB cable or hub:
- Unit MIDI switch:
- DIN receiver:

## Descriptors

- VID:
- PID:
- Product string:
- Interfaces:
- MIDI input endpoint:

## Results

- UART/DIN smoke:
- Bypass:
- Eight-pad learn and reboot:
- Held revoicing:
- Performance timing:
- Sustain and expression:
- Disconnect panic:
- 20 reconnect cycles:
- 30-minute stress:
- Maximum scheduler lateness:
- Maximum queue depth:
- Stuck notes:
