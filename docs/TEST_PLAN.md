# Test plan

## 1.1.0 BLE handshake, connections, help and MIDI files

Record the exact image hash, Cardputer revision/alias, cable/power, keyboard firmware, SD card and DIN receiver in SMK37_TEST_RESULTS.md. Upload verification is not runtime acceptance.

1. Diagnostic USB build: retain the boot trace. Start SMK-37 once, play and release notes. Expect READ → READ_RESULT status 0 → REGISTER/REGISTER_RESULT → CCCD_ON/WRITE_RESULT status 0 → READY → NOTIFY. An empty successful read is expected, not missing MIDI. Confirm amber BT before first decoded MIDI, green afterward. Repeat ten cold starts with Cardputer first and ten with keyboard first. Do not hide failed trials by restarting the keyboard before collecting logs.

   For the MTU fallback candidate, first inspect LINK: status is the library's stored connection ID, handle is the incoming ID. First boot is expected to show stored 255 versus incoming 0, followed by MTU_REQUEST status 0 and MTU status 0 with the negotiated size in handle. On a reused matching connection ID, the library requests MTU itself and the fallback must not issue another request. Capture both cold and warm cases; a successful exchange still requires played-note verification.
2. Test Tab BLE reconnect while connected, silent, playing and scanning; release/panic must precede reconnection work. Record whether advertising returns without keyboard power cycling. Test authentication-required devices separately; logs must show the read/security result and bounded timeout rather than indefinite SUB. Idle without notes must not trigger automatic subscription toggles/reconnects.
3. Test standard BLE-MIDI devices from other vendors; USB direct low-powered class-compliant input; Unit MIDI DIN IN → Grove GPIO1 at 31,250 baud with output switch SEPARATE. No external hub support. AUTO selects one source only; rejected sources and unrelated disconnects must not corrupt selected-source notes. Test running status, active sensing, realtime and UART overflow recovery.
4. Tab Layer channels defaults to four, clamps 1–16, survives idle-save/reboot and migrates old settings without losing mappings. L and the mapped layer action cycle 1–N. Esc opens all seven help pages, Tab Help reaches the same list, Fn+Esc always panics, and releases of held modifiers still work through menus. Current Options has 56 rows, ending in MIDI Player; old 51/58-row checklists refer to older builds.

   SD folder rename: on a fresh card, verify `/M5Chord/controllers`, `/M5Chord/presets`, `/M5Chord/loops` and `/M5Chord/logs` are created. On an existing card, manually rename `/midi-brain` to `/M5Chord` before this firmware boots; verify pad mappings, preset/loop loading and diagnostic exports use the new folder. Reselect a custom profile if necessary. Confirm no legacy SD folder is recreated, `/midi` playback is unchanged, and internal settings survive. Record physical results; no device-card rename is performed by the development tools.
5. Copy format-0 and format-1 `.mid`/`.midi` files to `/midi` on FAT32 SD. Include tempo changes, simultaneous notes, sparse source channels 2/4/6/8/10 and a sustained final note without Note Off. Tab MIDI Player → Load → choose with ;/. → Enter. Verify responsive validation progress, no automatic playback, persistent errors and Play selection after success. Enter starts/stops from the beginning. Check filename, elapsed/total time, 4/5 CH, long names, empty folder, missing card, invalid/truncated/oversized files and more than 32 entries. Exit during validation: completion must not force the player page back open. A rejected load must not leave a partially playable file. Test the user-supplied ghaetta.mid (50,582 bytes, 12,211 channel events, about 6:09) in full twice, including stop/restart and a different-file load. The file is not distributed in this repository.
6. With Layer channels=4, source 2/4/6/8 map to outputs 1/2/3/4 and source 10 is absent. With N=5, all five map to 1–5; with N=1 only source 2 plays on output 1. Multiple notes/tracks on one channel remain polyphonic. Changing N during playback stops and releases the file; next playback uses the new limit. Verify program changes, pressure, pitch bend and sustain use the same mapping.
7. Measure playback against an independent MIDI receiver: PPQN timing and tempo changes, default tempo, Note Off order, EOF cleanup and dense-event overload. Test overlapping live/file notes; stopping a file must not release a live-owned copy of the same pitch. Observe channel-wide pedal behavior and use separate live channels when needed. File start stops the looper; loop start stops the file. Fn+Esc and selected input disconnect stop file output and clear registry/scheduler.
8. Verify keyboard-view illumination follows file dispatch, not file loading or cached chord display. Stop or EOF must clear file-only keys. Instrument the background reader and MIDI task: SD reads must occur only in storage work, not MIDI event processing. Induce a read failure/slow-reader underrun and verify persistent error, stopped playback, released file notes and no stale events on restart. The card is now required during playback. Do not remove the card during writes or expect hot-insert remount without reboot.
9. Check free heap and main-task stack margin with BLE active, largest accepted file, all views and maximum menu activity. Confirm low-memory failure is safe. Diagnostic USB host is OFF; repeat direct USB tests with the normal universal build before release.
10. On a separate clean-settings test unit, verify first boot is CHORD plus Keyboard view. Do not erase the user's settings for this test. With saved KEY/BYPASS settings, verify upgrades retain that mode. Cycle to each view, stop/release notes and allow the 1.5-second idle save, then reboot and confirm the view. Missing/invalid view values fall back to Keyboard; loading a musical preset must not replace the view preference. Older firmware's unsaved view cannot be recovered.

Native tests include malformed-input mutations, truncated files, format/timing rejection, multi-track tempo merge, channel limiting, repeated-pitch ownership, missing-off/sustain cleanup, overflow/panic/disconnect and browser actions. These are software evidence, not DIN wire or LCD acceptance.

## Universal Cardputer compatibility — 2026-09-13

Run this on 1.0, 1.1 and ADV separately with the same universal application hash. Record the physical revision, MAC, power source, SD model, controller/firmware, cable/adapter, Unit MIDI switch and receiver in `SMK37_TEST_RESULTS.md`.

1. Cold boot on battery and USB power, with and without SD. Confirm a stable performance screen and Geek family `1.0/1.1` on both classic revisions or `ADV`. No revision selector is required. Unknown boards must not initialize MIDI or SD pins.
2. Test every shortcut and Tab Options. In CHORD, exchange held S for D and Q for W without an empty snapshot; the old modifier must release and the new one must activate. Hold a modifier while opening Options or pressing Fn, then release it; nothing should stick. Check real multi-key rollover separately from host snapshot tests.
3. Repeat all keyboard checks after mounting SD and after saving/loading presets and profiles. The classic keyboard's GPIO5 must remain under keyboard-driver control. Check SD-absent boot too.
4. Verify SMK-37 BLE input on the first connection, notification/event counters, cold-start order, disconnect panic and reconnect. Verify EasyPlay direct USB separately. External USB hubs remain unsupported; SMK USB interoperability is not implied by this build.
5. With Unit MIDI in SEPARATE, test bypass, chord/KEY, stacked/momentary pads, arp/strum and loop release on the DIN receiver. Panic and lane changes must clear notes. Check live keyboard animation against received messages.
6. Change settings, release notes/stop the loop, wait at least 1.5 seconds idle, reboot and verify persistence. Normal source upload preserves NVS regions; merged factory installation deliberately resets them. Back up before testing factory installation.

## Retro display and live keyboard — 2026-09-13

This replaces the visual expectations of the earlier DejaVu/READY/two-pad-row designs below. Keep the KEY-extension musical tests.

1. Confirm black/green/amber colors, angular large chord names, a quiet idle dash, a spaced single modifier rail, and no performance instructional text. Test C, C7, Cmaj7(b7), and long sharp-root/suspended/stacked names without clipping. Held versus selected quality must remain distinguishable before playing.
2. Check Tab Options, all 51 rows, long scale/mapping values, all five mapping-editor fields, learn modes, and the footer legend at real viewing distance. Navigation and mappings must not change.
3. In KEY C Major with only the performance stream enabled, test ARP at 120 BPM, 1/16, 50% gate: C, E, G should light individually at 125 ms steps with clear gate gaps. STRUM at 25 ms should add C, E, G in that order and keep gates lit until release. Test descending, random, two octaves, pattern, slop, harp and block. Amber attack marks last 50 ms; green gates follow Note Off, not chord-history linger.
4. Test very short gates, 2 ms strum, rapid roots, overlapping identical notes and root release before scheduled notes. Do not expect the LCD to separate events faster than its measured frame interval. Check panic, disconnect and lane change remove stale lights.
5. Enable raw/bass on different channels: the live keyboard should still isolate the performance channel. Play and stop loops on that channel while holding an overlapping live note. In BYPASS, test notes on several input channels.
6. Measure actual MIDI latency, frame rate and runtime heap/stack with the live keyboard active and BLE traffic. Verify cold-start SMK-37 input, stable display and saved mappings/settings. Record the exact device/power/cable/keyboard/receiver setup and results in SMK37_TEST_RESULTS.md.

Native preview command: use the optional preview build below, run `build-preview/midi_ui_preview build-preview/retro`, then `python3 tools/ui_contact_sheet.py build-preview/retro`. This creates stills, all menu/editor states, eight one-second output sequences, GIFs and sampled timelines. The contact-sheet script asserts static header/modifier regions remain identical across each sequence.

## KEY extensions and interface revision — 2026-09-13

Record the physical setup and observed results in `SMK37_TEST_RESULTS.md`. These checks supersede the older READY/blue-badge display checklist below.

1. In KEY, C Major, Latched, block mode, play C4. Check base C/E/G; hold 6 for A, m7 for B-flat, M7 for B, and 9 for D5. Hold both sevenths together, then release each independently. Repeat via mapped SMK pads, in stacking mode, with overlapping roots, and in arp/strum/pattern. Release and panic must leave no stuck or late notes.
2. Confirm `AUTO SCALE`, manual `OVERRIDE`, `SNAP ON/OFF`, and BYPASS `SCALE OFF` match the actual behavior. Explicit automatic KEY extensions may leave the scale. In CHORD with snap enabled, quantization can merge them.
3. Before playing, confirm `Play a note`, immediate pad feedback, named governing quality, and `EXT HOLD/TOGGLE`. Hold two qualities and distinguish held pads from the governing underlined selection. BYPASS dims every modifier.
4. Verify DejaVu Sans readability, all shortcut labels including W, full scale-status text, long chord names, Options navigation legend, all 51 rows, mapping editing, learning progress, and transient feedback without covering chord or pads.
5. Check release linger/fade, idle and loop-only states, panic, all four views, RX pulse versus connection indicators, and persisted settings after the normal idle-save interval.
6. Confirm BLE first-connection input and reconnect with the SMK-37, correct DIN notes on Ambient Zero, and stable startup. Measure latency and memory margins under load; native screenshots cannot establish these.

Optional actual-renderer visual regression preview (requires pinned PlatformIO libraries, SDL2 development files, and Pillow; the local Intel SDL2 installation requires x86_64):

```sh
cmake -S . -B build-preview -DMIDIBRAIN_BUILD_UI_PREVIEW=ON -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build build-preview --target midi_ui_preview -j 4
build-preview/midi_ui_preview build-preview/screens
python3 tools/ui_contact_sheet.py build-preview/screens
```

This runs the same UI code and bundled font bitmaps into a 240×135 framebuffer. It does not emulate keyboard hardware, BLE, LCD brightness, or DIN timing.

## Automated

Run strict host tests:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Build the target image:

```sh
pio run -e cardputer-adv
```

## SMK-37 BLE cold start and reconnect

1. Record SMK-37 model/firmware, Cardputer image hash, power sources, DIN receiver, and any other BLE hosts; disconnect other hosts from the keyboard.
2. With the SMK-37 already on, boot only the Cardputer. Wait for yellow setup to become green, then play and release keys without restarting the SMK-37. In Geek view, confirm `N` and `E` increase and `D` stays zero.
3. Repeat with Cardputer already running before powering on the SMK-37, and with both powered on together. Repeat each order at least five times.
4. Leave a newly connected keyboard idle for 30 seconds; confirm it stays connected, then test the first Note On and Note Off. Idle silence must not cause repeated reconnections.
5. Power off the SMK-37 while holding a chord; confirm DIN notes are silenced. Power it back on and confirm input resumes without rebooting the Cardputer. Repeat 20 times.
6. If green still appears without input, record the state and `N`/`E`/`D` values before restarting anything, and export diagnostics for subscription attempts/status. A successful descriptor acknowledgement alone does not establish hardware acceptance.

## Arabic scale selection

1. Record the image hash, keyboard connection, selected scale/key root, receiver, and receiver tuning. These scale options send standard 12-tone MIDI notes without tuning commands.
2. Select KEY mode, release all quality pads, set key root C, and choose each new `12T` scale in Options. Verify the key label changes and the Scale menu wraps both ways through all 12 entries.
3. With extensions off and voicing zero, play MIDI note 61 in Hijaz: expect 61/65/68. For an audible distinction between all choices, play multiple degrees rather than only the tonic triad.
4. Choose ARP and hold/release roots. Check for scale-contained output and no stuck notes. Switch scale while holding a root, then release; repeat with overlapping roots.
5. Leave the device idle for settings save, reboot, and confirm the scale/key remain selected. Save/load a preset with a new scale, then load an older preset and confirm its original scale remains unchanged.

## Unit MIDI

1. Set the Unit switch to `SEPARATE`.
2. Connect Grove and DIN output to a MIDI monitor.
3. Send channel 1 C4 Note On velocity 100 and Note Off from a temporary diagnostic build or USB bypass source.
4. Confirm exact bytes `90 3C 64` and `80 3C 00` at the receiver.

## SMK-37 USB

1. Charge the controller fully.
2. Open Geek view and attach it through a known-good path.
3. Record VID, PID, interface number, endpoint, and whether audio interfaces coexist.
4. Exercise notes, pitch, modulation, sustain, pads, encoders, faders, pressure, and transport controls.
5. Disconnect and reconnect 20 times.

## Bridge

Test a chromatic scale, rapid repeats, overlapping identical pitches, 10-note clusters, sustain transitions, expression controls, panic, and removal during a held chord. Repeat in bypass, block chord, strum, arp, and harp modes.

No emitted note may remain active after all sources release or panic. No delayed Note On may fire after its source release.

## Timing

Measure from USB-MIDI receive to first UART byte. Record typical, p95, and maximum values for bypass and block chord. Measure scheduler jitter with UI idle and while changing views. Acceptance is under 8 ms for normal bypass, under 4 ms chord scheduling, and under 2 ms rhythmic jitter.

## Endurance

Run ordinary playing, sustain, rapid parameter changes, and periodic reconnects for 30 minutes. Record resets, dropped events, maximum queue depth, maximum scheduler lateness, panic count, and any stuck notes.

## Controller expansion acceptance — flashing on hold

Do not execute device tests until the user authorizes flashing. Record the exact setup and each result in `SMK37_TEST_RESULTS.md`; a successful build is not physical acceptance.

1. Hold S, add D, then release S while holding Fn. Only the original m7 release should occur; D stays active. Repeat with one MIDI pad and one Cardputer key mapped to the same extension.
2. Confirm Fn+S saves a preset without adding m7, Fn+L loads without changing lanes, and L only changes the performance lane. Fn+Esc must panic from every page and learn mode.
3. In Options, verify arrows only adjust/select; deletion, load/save and other commands require Enter. Confirm no performance shortcuts leak through the menu and held-note releases still arrive.
4. Learn the nine pads with rapid repeats and long holds. The wizard must wait for release and reject duplicates. Finish, confirm the save result, power-cycle, and test all mappings.
5. Learn and edit a CC encoder, all three relative encodings, a program-change action, and a note pad. Delete a mapping, save/reload its profile, and verify consumed controls do not sound downstream.
6. Test Simple/Advanced/Free/Latched and extension Add Note/Retrigger. Verify source/fixed velocity, transpose, key learning, manual Key override, input filters, and independent stream channels.
7. Record free and 1/2/4/8/16-bar loops, quantize, overdub, undo and clear. Hold a live note identical to a loop note while stopping playback. Verify no premature live Note Off or hanging loop note.
8. Save/load all slot types, reject corrupt/out-of-range files, try missing/full SD, and power interruption during file replacement. Verify a pending save keeps its requested slot even if slot selection changes.
9. Export the 64-sample diagnostics history after dense MIDI traffic. Measure BLE/USB input latency, UART congestion, available heap and task-stack margin with SD operations and the display active.
10. Repeat BLE overflow/disconnect/reconnect and direct USB reconnection under held notes. Check that stale Note Ons cannot replay after panic. Direct SMK USB and external hubs remain unresolved/unsupported respectively.

## Display-feedback revision — build only

After a separately authorized upload, verify idle Key mode shows a small labelled key/scale and large `READY`, not a large C-major chord. Before playing any root, hold/release each quality and extension pad and check the corresponding green badge. Check multiple held extensions and a shared Cardputer/MIDI extension; releasing one source must not remove the badge while another holds it. Enable stacking and confirm blue badges survive pad release and toggle off on the next press.

Play/release a chord and verify a 1.2-second linger with a final 0.5-second fade. Hold an arpeggiated root across rests and verify the harmony stays visible. Panic must clear the hero and stale note details immediately. Repeat on chord, notes, keyboard, and diagnostics views, including a parameter overlay. Check all text and badge rows fit the physical 240×135 display. No hardware visual acceptance is implied by host tests or a firmware build.

Optional host memory/undefined-behavior check:

```sh
cmake -S . -B build-sanitized -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-sanitized
ctest --test-dir build-sanitized --output-on-failure
```
