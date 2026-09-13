# Implementation plan

## MIDI Player streaming and interface follow-up

- [x] Replace device whole-file event loading with background validation and a fixed read-ahead queue; keep SD outside MIDI event processing.
- [x] Consolidate Tab commands into one MIDI Player page with Load/Play/Stop, persistent errors, validation progress, elapsed/total time and used/available channels.
- [x] Add native full-song/replay diagnostics and regression coverage for buffering, parser limits, cache boundaries, underrun/read-failure cleanup and navigation.
- [x] Use CHORD and Keyboard view for fresh settings, preserve saved modes, and persist view choice separately without changing musical preset schemas.
- [ ] Record physical full-song playback, responsive loading and slow/removed-card recovery on each supported Cardputer revision before release acceptance.

## 1.1.0 development sequence

- [x] Rename all SD app-data paths to `/M5Chord`, centralize path definitions and validate remembered profile paths. Keep `/midi` and internal settings unchanged; document manual SD folder rename, with no automatic migration.
- [x] Generalize standard BLE-MIDI discovery, explicit AUTO/BLE/USB/DIN selection and Unit MIDI DIN decoding.
- [x] Add persistent 1–16 layer-channel count (default four), schema-2 migration, and paged Esc/Tab shortcut help.
- [x] Capture the SMK-37 startup failure and failed reconnect; record physical setup and successful keyboard-restart workaround.
- [x] Implement the missing asynchronous initial MIDI characteristic read, bounded authentication retry, richer BLE trace and decoded-input indicator. Remove the unsuccessful silent subscription toggle.
- [x] Add bounded portable SMF 0/1 PPQN decoding, tempo-map merge, channel selection/remapping and File-stream playback through the existing scheduler/registry.
- [x] Add idle-only `/midi` SD loading and a Tab filename browser, play/stop commands and no new physical shortcuts.
- [x] Document scope, limits, migration and all recent changes in README and draft 1.1.0 release notes; update version-aware packaging/CI.
- [x] Complete final build/test/visual checks and flash the requested diagnostic candidate without erasing saved-settings storage; see the exact hash/setup in SMK37_TEST_RESULTS.md.
- [ ] Complete individually recorded first-connection BLE, SD playback/channel limits, DIN input and persistence checks. The user approved the diagnostic build and authorized 1.1.0 publication with the remaining coverage and BLE issue documented.

## Completed software gates

- [x] Build a universal 1.0/1.1/ADV candidate with runtime keyboard selection, shared safe pin definitions, same-count key-swap regressions and verified factory/application packages.
- [x] Apply the retro instrument redesign and replace cached keyboard illumination with bounded, dispatch-driven output gates and attack markers.

- [x] Correct independent KEY-mode 6/m7/M7/9 additions and add interval/lifecycle regressions.
- [x] Redesign device UI with consistent DejaVu Sans, explicit scale/quality/hold state, contextual Options, and native framebuffer previews.

- [x] Create a reproducible PlatformIO Cardputer ADV firmware target.
- [x] Separate the pure C++ musical engine from Arduino, M5Stack, USB, display, SD, and NVS APIs.
- [x] Implement normalized MIDI events and complete-status-byte UART serialization.
- [x] Implement major, minor, diminished, suspended, 6, m7, M7, 9, scales, diatonic harmony, quantization, and inversion voicing.
- [x] Implement bounded stable scheduling with owner and stream cancellation plus lateness counters.
- [x] Implement block, strum, two-octave strum, deterministic slop, arp, two-octave arp, pattern, and harp timing without delays.
- [x] Implement voice ownership, overlapping-pitch reference counting, common-tone revoicing, disconnect cleanup, and global panic.
- [x] Implement bypass, chord, and key routing plus expression broadcast, bass, raw-chord infrastructure, and MIDI clock.
- [x] Implement native composite-safe USB MIDI interface discovery and asynchronous input transfers.
- [x] Implement Cardputer shortcuts, chord-centric views, contextual overlays, Geek diagnostics, guarded nine-pad learning, and four-layer output selection.
- [x] Implement bounded SD JSON profile parsing, atomic wizard saves, Generic fallback, and NVS settings persistence.
- [x] Pass strict host tests and the pinned ESP32-S3 firmware build.

## Physical acceptance gates

- [ ] Validate the same universal image on Cardputer 1.0, 1.1 and ADV: cold boot, keyboard/SD coexistence, BLE, direct USB and DIN lifecycle. A successful upload is not functional acceptance.
- [ ] Confirm Cardputer UART output through Unit MIDI in `SEPARATE` mode.
- [ ] Capture the SMK-37 descriptors and verify its composite interface topology.
- [ ] Verify transparent bypass for notes, velocity, sustain, pitch bend, modulation, CC, pressure, and program change.
- [ ] Complete the real nine-pad wizard and reboot persistence check.
- [ ] Measure bypass, block, and scheduler latency against the specification.
- [ ] Complete 20 USB reconnect cycles and a 30-minute stuck-note stress run.
- [ ] Record results in `docs/SMK37_TEST_RESULTS.md`.

## Controller expansion software

- [x] Central conflict-checked keymap, exact modifiers, key edges and original-action release ownership.
- [x] Options parameters and explicit-confirmation commands, shared with mapped MIDI actions.
- [x] Simple/Advanced/Free plus backward-compatible Latched interaction, extension retrigger policy, key learning, transpose and velocity control.
- [x] General MIDI Learn action/trigger/relative/consume editor, mapping edit/delete, and release-gated nine-pad wizard.
- [x] Profile selection/reload/save and USB identity/BLE-name matching, strict portable validation, input filters and Generic fallback.
- [x] Bounded post-engine MIDI loops: free/fixed length, record/play/stop, overdub, latest-layer undo, clear and onset quantization.
- [x] 16 controller preset slots and loop files, versioned/checksummed codecs and temporary/backup replacement.
- [x] NVS migration and deferred persistence outside MIDI event processing.
- [x] Fixed diagnostics history and idle SD export.
- [x] Conservative expression coalescing, Note On preference over replaceable controls, DIN critical-overflow panic and BLE overflow/disconnect cleanup.
- [x] Portable regression tests including mapping UI, all action-name round trips, loops, codecs, transport congestion, and modifier conflicts.
- [x] Controller-only ORC-1 assessment with intentional differences documented in `docs/ORC1_CONTROLLER_ASSESSMENT.md`.

## Remaining gates and refinements

- [ ] Physical verification of this expansion; the subsequent authorized upload passed flash hash verification, but runtime acceptance remains pending.
- [ ] Hardware-derived queue thresholds, latency measurements, keyboard rollover and long-running stress.
- [ ] SD reboot persistence, interrupted-write recovery, removal/full-card behavior and memory headroom with BLE active.
- [ ] Direct SMK-37 USB enumeration investigation; active-configuration discovery is not a promise of every composite-device topology.
- [ ] Optional ORC-specific refinements: pattern library, count-in, external-clock following, loop time-stretch, exact voicing tables and additional bass policies.

Synth/audio/drum/effects implementation remains out of scope.
