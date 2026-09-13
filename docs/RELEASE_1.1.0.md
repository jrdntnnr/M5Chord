# M5Chord 1.1.0

Feature release after 1.0, authorized by the user after approving the preceding diagnostic build. Firmware identifies as `1.1.0`. The download is the normal universal image for Cardputer 1.0, 1.1 and ADV, with USB MIDI host enabled—not the USB-serial diagnostic image. The SMK-37 startup issue remains a known limitation; approval does not establish exhaustive hardware acceptance.

## Added

- SD Standard MIDI File playback: `/midi`, Tab → MIDI Player, a bounded filename browser and a single dedicated page with Load and Play/Stop, filename, persistent status, loading progress, elapsed/total time and used/available channels. Loading is separate from playing. Removed the redundant separate Stop menu entry; MIDI-mapped stop remains available. No new physical performance shortcuts.
- Background validation and buffered SD playback replace the device's whole-song 2,048-event limit and blocking load. A fixed 512-event queue feeds the musical engine; SD read errors and underruns stop safely with a persistent explanation. A read-only native checker exercises full-file validation, replay, channel limiting and note cleanup.
- First-install CHORD mode and Keyboard view. Existing saved modes are respected; display-view selection now saves independently in NVS after an idle debounce. Older firmware did not persist the view, so installations without a stored view start in Keyboard view. No preset schema change for this preference.
- Channel-limited file playback: the lowest-numbered source channels containing notes map to outputs 1–N; extra channels are omitted, not folded into occupied outputs. Default N is four. Chords retain polyphony within each channel; tracks are not voices. Channel 10 is not reserved for percussion.
- MIDI input selection: AUTO, BLE, USB or DIN. AUTO selects the first input with a channel message and excludes others until that selected input disconnects or selection changes. This avoids cross-input note ownership collisions; it is not an input merger.
- Unit MIDI/SAM2695-board DIN input at 31,250 baud through Grove RX, with portable running-status/realtime decoding, active-sensing recovery and bounded UART polling. Output remains through DIN in SEPARATE mode; no synth/audio features were added.
- Vendor-independent discovery of standard advertised BLE-MIDI services; case-insensitive MIDI-name and legacy SMK BLE identity fallbacks, with GATT validation. One device at a time; no BLE device chooser, Bluetooth Classic or proprietary-protocol guarantee.
- Tab → Layer channels, saved from 1–16, default four, controlling L/mapped layer cycling and file playback's output-channel limit.
- Esc and Tab → Help / shortcuts open seven pages covering every physical binding. Menu arrows page through help; Fn + Esc panic and modifier-release behavior remain intact.
- Bounded BLE trace export and an optional USB diagnostic build/logger. USB serial diagnostic firmware deliberately disables USB MIDI host mode; it must not be distributed as the normal universal image.

## Bluetooth changes and known issue

The new connection sequence reads the MIDI I/O characteristic before enabling notifications, as specified by BLE-MIDI's initial connection procedure. Read, registration and descriptor writes are acknowledgement-driven with timeouts. An authentication-required read can request encryption and retry once. The prior automatic two-second subscription toggle was reproduced failing and is removed.

Logs now include read results, MTU exchange, security/authentication, connection-parameter updates and sampled rejected notifications. The BT indicator is amber until a MIDI event is decoded in the current session, then green; `ON` in Geek view still describes an acknowledged subscription. Silence is not a reason to repeatedly disconnect an idle instrument.

Follow-up hardware capture confirmed that the initial read alone did not fix silent first-session input. Source inspection then found that the pinned BLEClient can skip MTU exchange on the first connection: CONNECT compares against an old connection ID, which is only updated in OPEN. A guarded fallback now requests MTU only when that library path would be skipped. LINK logs the stored and new IDs; MTU_REQUEST logs the fallback result, and MTU logs completion. This remains a separate physical acceptance candidate, not a confirmed root cause or fixed-bug claim.

The existing SMK-37 startup bug remains unresolved. Prior diagnostic tests reproduced no input despite successful subscription and failed Cardputer-side rediscovery after manual reconnect; restarting the keyboard restored discovery and input. Workaround: switch the keyboard off and back on twice while M5Chord stays running. Keep this workaround until the handshake changes pass repeated first-note tests. Detailed setup/results are in [SMK37_TEST_RESULTS.md](SMK37_TEST_RESULTS.md).

## File playback scope and safety

- Format 0/1, PPQN timing, running status, tempo maps, notes, CC below 120, program changes, pressure and pitch bend. One-shot playback at file tempo, independent of live BPM/clock.
- Limits: 1 MiB, 32 tracks, 256 pending note owners, 24-hour duration. Browser: 32 files, 63-byte names, at most 512 directory entries. Keep SD inserted: a background task reads ahead during playback; SD, allocation and UI remain outside MIDI event processing. The bounded in-memory parser helper used in native fixtures still has its separate 2,048-event capacity; the hardware loader does not use that helper.
- SysEx and non-tempo metadata are skipped; format 2, SMPTE, nonzero MIDI-port metadata, RIFF/RMID and MIDI 2.0 files are rejected. No pause/seek/repeat/tempo override or automatic reload after reboot.
- Every file note uses `VoiceId`, a dedicated File stream, `MidiScheduler`, `ActiveNoteRegistry` and the existing output activity observer. Stop, EOF, panic, channel-count change and overload clean up file notes. Shared channel sustain remains channel-wide; file stop resets sustain, sostenuto and hold-2 on file outputs.
- File playback and the internal looper are mutually exclusive. Live playing may accompany a started file. File program/controller changes affect the receiver; channel-mode CC 120–127 are suppressed.

## Compatibility, settings and documentation

SD app data now lives under **`/M5Chord`**: controller profiles, presets, recorded loops and logs. New installations create the folder tree automatically. Existing users must manually rename `/midi-brain` to `/M5Chord`, preserving its contents, when installing this firmware. There is no automatic folder migration or legacy-path fallback. A custom selected profile may need reselection. Playback `.mid` files remain in `/midi`; internal settings storage and its existing namespace are unchanged.

The universal build retains runtime support for Cardputer 1.0, 1.1 and ADV without assuming PSRAM. M5Chord remains the startup title, BLE local name and diagnostic app name.

Settings schema 3 adds AUTO input and four layer channels when importing schema 2. Internal saves use `settings-v3` and retain `settings-v2` for downgrade; 1.0 sees its earlier record, not subsequent changes. New schema-3 SD presets are not readable by 1.0. Loaded MIDI files are not presets and are not saved in NVS.

README now explains all new controls, wiring, MIDI-file channel mapping and limits, migration and diagnostic logging. USB input is described as suitable for low-powered, simple class-compliant controllers connected directly; compatibility/power testing is still device-specific. External hubs, including powered hubs, remain unsupported by the pinned host stack. SMK-37 USB support is not claimed.

Packaging derives its version from AppInfo, requires a clean source tree and rejects diagnostic firmware masquerading as a universal image. CI builds all three environments. GitHub assets include the universal factory image, application and settings-preserving update components, checksums, manifest and complete ZIP. M5Burner catalog publication has not been performed.

## Verification and remaining hardware coverage

Local development verification completed on 2026-09-13: strict and sanitized CTest, all three pinned PlatformIO builds, ten packaging regression tests and native UI previews passed. The requested diagnostic upload passed all flash hash checks on ADV-TEST-01. Exact hashes and limits of that evidence are in SMK37_TEST_RESULTS.md; the user subsequently reported the build good and requested 1.1.0. The checks below remain unverified individually and are not implied by that general approval.

- [ ] Repeated cold starts in both Cardputer/SMK-37 power-on orders: first played note works, no keyboard restart.
- [ ] Manual reconnect, keyboard disconnect/restart, held-note release, panic and no-data idle tests with retained traces.
- [ ] SD file load/play/stop on real hardware, tempo-changing and five-channel files at N=4 and N=5, receiver channel verification, card absent/removed/invalid-file behavior.
- [ ] Unit MIDI DIN input and simultaneous live/file output tested with a recorded physical setup.
- [ ] Help/menu navigation, no shortcut conflicts, saved layer count and schema-2 migration verified on hardware.
- [ ] Heap/stack and dense MIDI output measured with BLE and display active; repeat relevant checks on 1.0, 1.1 and ADV.

See [README](../README.md) for full instructions and [TEST_PLAN](TEST_PLAN.md) for the acceptance procedure.
