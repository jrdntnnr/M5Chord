# M5Chord

M5Chord is a standalone harmonic MIDI controller for M5Stack Cardputer 1.0, 1.1 and ADV. Connect a USB-MIDI or Bluetooth MIDI keyboard, turn single notes into chords, add extensions, play arpeggios and strums, and send the result to your instruments over DIN MIDI.

It produces **MIDI, not audio**. An M5Stack Unit MIDI in `SEPARATE` mode provides the DIN output. No computer is needed while playing.

[Download M5Chord 1.0](https://github.com/jrdntnnr/M5Chord/releases/tag/v1.0) · [Report an issue](https://github.com/jrdntnnr/M5Chord/issues) · [Release notes](docs/RELEASE_1.0.md)

> **Known Bluetooth bug — SMK-37:** the keyboard can appear connected while its keys do not register. The current workaround is to switch the **SMK-37 off and back on twice** while M5Chord stays running, then check that MIDI receive counters increase. A fix is planned for a coming version; it is **not fixed in 1.0**. See [Bluetooth connection](#ble-midi-connection).

## Contents

- [Hardware and wiring](#hardware)
- [Install the 1.0 binary](#install-the-10-binary)
- [Quick start: chords and arp](#quick-start-chords-and-arp)
- [USB connection](#first-connection) and [Bluetooth connection](#ble-midi-connection)
- [Every Cardputer key](#controls)
- [Modes, playstyles and Options](#options-and-playstyles)
- [Scales](#arabic-scale-options)
- [Pad setup and MIDI Learn](#nine-pad-setup)
- [Profiles](#controller-profiles), [loops and presets](#midi-loops-and-presets)
- [Display feedback](#display-feedback) and [troubleshooting](#troubleshooting)
- [Build from source](#build-and-flash) and [tests](#desktop-tests)

## Compatibility

One universal binary selects the appropriate keyboard driver automatically. No revision setting is needed.

| Cardputer | Driver | Current evidence |
|---|---|---|
| 1.0 | GPIO keyboard matrix | Compatibility build flashed and reported working by the user |
| 1.1 | Same GPIO keyboard matrix | Supported by shared wiring/driver; physical test still pending |
| ADV | TCA8418 keyboard | Earlier builds used on hardware; complete 1.0-release acceptance still pending |

The 1.0 release adds the M5Chord name/version to the working compatibility code. Do not interpret build success as exhaustive hardware validation. See [compatibility details](docs/COMPATIBILITY.md) and [recorded results](docs/SMK37_TEST_RESULTS.md). This project is independent of M5Stack and is not an official M5Stack product.

## Signal path

```text
SMK-37 USB-C or BLE MIDI
           │
           ▼
M5Chord / Cardputer 1.0, 1.1 or ADV
     │
     │ Grove:
     │ GPIO2 = MIDI TX
     │ GPIO1 = MIDI RX
     ▼
M5 Unit MIDI [SEPARATE]
     │
     ▼
DIN MIDI OUT
     │
     ▼
External MIDI device
```

## Hardware

- M5Stack Cardputer 1.0, 1.1 or ADV
- M5Stack Unit MIDI with its front switch set to `SEPARATE`
- A standard BLE-MIDI controller, or a class-compliant USB-MIDI controller
- DIN-MIDI receiver, synth, effects processor, or other instrument
- Known-good USB data cable
- Direct USB OTG data path; use a single-device USB 2.0 power injector if the controller requires external VBUS
- Optional FAT32 microSD card for controller profiles, presets, loops, and diagnostics

The Grove connection is GND to GND, 5 V to 5 V, Cardputer GPIO2/TX to Unit UART_RX, and Cardputer GPIO1/RX to Unit UART_TX. The firmware configures UART2 for 31,250 baud, 8N1, RX GPIO1, and TX GPIO2.

Use one input controller at a time. For the SMK-37, use Bluetooth MIDI with the workaround above. EasyPlay1 Plus has worked over direct USB when placed in its MIDI mode. Direct SMK-37 USB interoperability is not established. A powered USB hub does not solve this: external hubs are unsupported by the current USB host stack.

## Install the 1.0 binary

Download and extract `M5Chord-v1.0.zip` from the [1.0 release](https://github.com/jrdntnnr/M5Chord/releases/tag/v1.0). It contains the universal image, separate update components, checksums, instructions and third-party notices.

| File | Address | Purpose |
|---|---|---|
| `M5Chord-v1.0-universal.bin` | `0x0000` | Fresh installation on any supported Cardputer; resets internal settings |
| `M5Chord-v1.0-app.bin` | `0x10000` | Application component; never flash this at zero |
| `bootloader.bin`, `partitions.bin`, `boot_app0.bin` | See update command | Settings-preserving update alongside the app |
| `SHA256SUMS`, `manifest.json` | Not flashed | Integrity checks and exact image metadata |

The merged universal image overwrites saved NVS settings. It does not erase your microSD card or the entire flash chip. Back up before fresh installation. Use the separate-component command below to preserve internal settings when updating this project. Other firmware's settings/layout are not promised to be compatible.

### Enter download mode

1. Disconnect controllers from the Cardputer USB-C socket and switch the Cardputer off.
2. Hold **G0**, connect a USB data cable from the Cardputer to your computer, then release G0.
3. Select its serial port: typically `/dev/cu.usbmodem…` on macOS, `/dev/ttyACM…` on Linux or `COM…` on Windows. Close serial monitors before flashing.

### Install the flashing tool

The commands below use esptool 4.5.1. In a terminal, create a Python environment:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install esptool==4.5.1
python -m serial.tools.list_ports
```

On Windows, use `py -m venv .venv`, then `.venv\Scripts\activate.bat` in Command Prompt (or `.venv\Scripts\Activate.ps1` in PowerShell). The remaining `python -m ...` commands are the same.

### Flash a fresh installation

Run from the extracted release directory. Replace `PORT` with the serial port identified above:

```sh
python -m esptool --chip esp32s3 --port PORT --baud 460800 write_flash 0x0000 M5Chord-v1.0-universal.bin
```

### Update while retaining M5Chord settings

Use this instead of the merged image when updating an existing build of this project:

```sh
python -m esptool --chip esp32s3 --port PORT --baud 460800 write_flash 0x0000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 M5Chord-v1.0-app.bin
```

This selects app0 and writes only the programming regions, leaving NVS untouched. App-only flashing is for advanced users who have verified the same `default_8MB` partition layout and active app0 slot. Do not use erase-all or change the flash settings supplied by the binary.

Expect writing progress and `Hash of data verified.` On completion, release G0 and power-cycle the Cardputer. You should see **M5Chord / V1.0** during startup, followed by the performance screen. Fresh settings start in BYPASS. Press Tab to open Options, and `V` to cycle to Geek view: `1.0/1.1` or `ADV` identifies the keyboard family.

The programming port normally disappears when the app switches native USB into host mode. It is not a USB-MIDI output or serial console while playing. If upload fails, check the data cable, re-enter G0 download mode and retry at `--baud 115200`.

Verify downloads with `shasum -a 256 -c SHA256SUMS` on macOS or `sha256sum -c SHA256SUMS` on Linux. On Windows, compare `Get-FileHash FILE -Algorithm SHA256` with the corresponding checksum. Flash-address behavior follows the [official esptool instructions](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-commands.html).

M5Burner catalog publication has not been performed. The merged image is packaged for installation at zero, but do not assume the project is already listed in M5Burner.

## Quick start: chords and arp

1. Connect Unit MIDI in `SEPARATE` mode and its DIN OUT to your instrument's MIDI IN. Set the receiver to channel 1 initially.
2. Connect a controller over BLE or direct USB. Check BYPASS first: notes should reach the receiver, and the receive dot/counters should react.
3. Press `M` until CHORD. Open Tab Options, set **Play style = LATCHED**, **Performance = BLOCK**, **Performance on = ON**, **Performance ch = 1**, and **Harmonic quantize = OFF**. Close Tab.
4. Tap `E` for major, then play a root on the external keyboard. C should produce C/E/G. Tap `W` for minor. The Cardputer letter keys select harmony; they are not a piano keyboard.
5. Hold `F` while playing for a ninth. Hold `S` for m7 or `D` for M7. Hold several extension keys/pads together to combine them. Press `X` for STACK if you prefer toggling extensions on and off with taps.
6. For arp, press `P` until ARP; hold a root. In Options, start with BPM 120, Rate 1/16, Gate 50%, Direction UP. The arp sequences the generated chord, not every scale degree. Press `V` to see individual output-key attacks and releases.
7. For automatic scale harmony, select KEY with `M` or `K`, then choose Key root and Scale in Options. Release any held quality pads when trying automatic harmony.
8. If anything hangs, press **Fn + Esc/backtick** for panic. For an unexpected single note in CHORD, check Play style: SIMPLE, ADVANCED and FREE start from a single note unless you hold a quality.

Quality choices (DIM/MIN/MAJ/SUS) are mutually exclusive chord types, not stackable extensions. The 6/m7/M7/9 modifiers can combine. The display indicates the governing quality and active additions.

## Build and flash

The build is pinned to PlatformIO Espressif32 6.7.0, Arduino-ESP32 2.0.16, M5Cardputer 1.1.1, M5Unified 0.2.21, M5GFX 0.2.28, IRremote 4.7.1, and ArduinoJson 7.4.3.

```sh
pio run -e cardputer-universal
pio run -e cardputer-universal -t upload
```

If PlatformIO cannot write its normal package directory, select a writable project-local core directory:

```sh
PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-universal
```

To enter Cardputer download mode, switch it off, hold G0, connect the USB-C data cable to the computer, release G0, and upload. Disconnect the programming cable before using the same native USB connector as a host.

The generated application image is `.pio/build/cardputer-universal/firmware.bin` at offset `0x10000`. The legacy `cardputer-adv` environment remains a build alias with the same shared configuration. Do not flash the application at zero. Clone the source with `git clone https://github.com/jrdntnnr/M5Chord.git`, enter `M5Chord`, and install PlatformIO Core before building. To reproduce the 1.0 source, select tag `v1.0`.

To package a release, use the Python interpreter running PlatformIO with its esptool dependencies available:

```sh
python tools/package_firmware.py --version 1.0 --output dist/M5Chord-v1.0
python -m unittest discover -s tools -p 'test_package_firmware.py'
```

Use `--core-dir` if PlatformIO's packages are not in `.pio`. The package script requires a new output directory and never reads flash from a connected device.

## Desktop tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Install the pinned PlatformIO dependencies first; host JSON tests use the same ArduinoJson headers. An alternative location can be supplied with `-DARDUINOJSON_INCLUDE=/absolute/path/to/ArduinoJson/src`.

Tests cover harmony, transport parsing, scheduling, note ownership, panic, exact-modifier keyboard edges, menu confirmation, MIDI Learn/edit/delete, playstyles, overlapping extension controls, profile/preset/loop validation, recording/overdub/undo, shared live/loop notes, diagnostic ring wrap, and DIN congestion recovery.

## First connection

1. Fully charge the SMK-37.
2. Set the Unit MIDI switch to `SEPARATE`.
3. Connect the Grove cable and DIN MIDI receiver.
4. Boot the Cardputer.
5. Connect a compatible USB-MIDI controller directly to its USB-C host port. Use the BLE section below for SMK-37.
6. Confirm `USB` turns green. Use `V` to reach Geek view and inspect VID, PID, MIDI interface, endpoint, queue, lateness, and active-note data.
7. Leave the first-boot mode at `BYPASS` and verify keys, velocity, sustain, pitch bend, modulation, CC, program change, and pressure.
8. Press `M` to enter `CHORD`, choose harmony, and test note release before enabling rhythmic performance modes.

Repeated resets, attachment loops, or intermittent enumeration usually indicate USB power or cable trouble. The pinned ESP-IDF 4.4 host stack does not support external USB hubs. Use a direct connection or a single-device USB 2.0 power injector that does not introduce a hub.

## BLE MIDI connection

**SMK-37 startup bug in 1.0:** a green connection indicator does not guarantee key input. The user-reported workaround is to switch the SMK-37 off and back on **twice**, leaving M5Chord running. Wait for reconnection and play a key after the cycles. This is a known bug scheduled for a coming version, not a permanent setup requirement or a guaranteed remedy for every connection problem.

1. Disconnect or disable any Mac, phone, or tablet already connected to the SMK-37 BLE MIDI endpoint.
2. Enable Bluetooth MIDI on the SMK-37. Its MIDI identity is normally `SMK-37 Pro_BLE`.
3. Boot the Cardputer. It scans automatically and does not require a pairing menu or passkey.
4. Confirm `BLE` changes from yellow while scanning/setting up to green after the keyboard acknowledges the MIDI notification subscription.
5. Use `V` to reach Geek view. `BLE:SUB` means subscription setup; `BLE:ON` means subscription accepted, not proof that keys are transmitting. `N` counts notifications, `E` counts decoded MIDI events, and `D` counts queue drops. Playing keys should increase `N` and `E`.

The existing subscription retries and refresh do not fully resolve the reported SMK-37 startup issue. Geek `N`/`E` counters, not the green connection label alone, confirm received notifications/events. Diagnostic exports include subscription attempts and GATT/API status for follow-up investigation.

USB and BLE inputs share the same MIDI parser, controller mappings, musical engine, scheduler, active-note registry, and panic behavior. A BLE disconnect invokes panic before automatic reconnection scanning begins.

## Arabic scale options

Western choices are **Major, Natural Minor, Dorian, Mixolydian, Harmonic Minor, Major Pentatonic, Minor Pentatonic and Chromatic**. All are available through Tab → Scale. KEY uses the scale automatically; CHORD uses it when harmonic quantization is enabled; BYPASS ignores it.

Options → Scale includes `Hijaz 12T`, `Hijazkar 12T`, `Kurd 12T`, and `Nikriz 12T`, after the original eight scales. Original scale IDs and saved selections are unchanged. The new selections use the existing automatic settings save and preset save/load paths.

| Scale | Semitones above the selected key root |
|---|---|
| Hijaz 12T | 0, 1, 4, 5, 7, 8, 10 |
| Hijazkar 12T | 0, 1, 4, 5, 7, 8, 11 |
| Kurd 12T | 0, 1, 3, 5, 7, 8, 10 |
| Nikriz 12T | 0, 2, 3, 6, 7, 9, 10 |

These are fixed 12-tone equal-tempered scale approximations, not a complete maqam implementation: no quarter-tone output, regional intonation, melodic development rules, or automatic modulation. Hijaz uses its Nahawand upper-jins variant. Pitch-set references: MaqamWorld [Hijaz](https://www.maqamworld.com/en/maqam/hijaz.php), [Hijazkar](https://www.maqamworld.com/en/maqam/hijazkar.php), [Kurd](https://www.maqamworld.com/en/maqam/kurd.php), and [Nikriz](https://www.maqamworld.com/en/maqam/nikriz.php). The semitone tables above are this firmware's equal-tempered realization.

To try them: select KEY mode with `M`; open Options with Tab; choose Key root and Scale using `;` / `.` to navigate and `,` / `/` to adjust; close Options. Press `P` until ARP and hold a keyboard note. Arp sequences the generated scale-derived chord, not all seven scale degrees. Holding a quality pad overrides KEY-mode harmony; release quality pads when auditioning the scale. In CHORD mode, a scale affects generated chords only when harmonic quantization (`H`) is enabled. BYPASS ignores the scale. There are no new shortcuts or binding conflicts.

## Controls

| Key | Action |
|---|---|
| Q / W / E / R | Diminished / minor / major / suspended |
| A / S / D / F | Hold 6 / m7 / M7 / 9 in momentary mode; toggle them in stack mode |
| `[` / `]` | Voicing down / up |
| M | Cycle bypass / chord / key mode |
| K | Toggle key mode |
| P | Cycle block, strum, two-octave strum, slop, arp, two-octave arp, pattern, and harp |
| B | Toggle root bass on channel 2 |
| T | Tap tempo; four plausible taps establish BPM |
| C | Toggle MIDI clock and send Start/Stop |
| H | Toggle manual harmonic quantization |
| V | Cycle chord, notes, keyboard, and Geek views |
| L | Cycle the primary output through Ambient Zero layers 1–4 |
| X | Toggle momentary / stack extension behavior and clear active extensions |
| O | Start the nine-pad SMK setup wizard |
| Tab | Open/close Options |
| `;` / `.` | Previous/next Options row |
| `,` / `/` | Decrease/increase a parameter; select mapping index |
| Enter | Confirm the selected command |
| Esc/backtick | Leave Options or cancel learning |
| Z | Start a new loop recording; press again to finish recording and play |
| Space | Play/stop loop; finish recording and play |
| Fn + Z | Toggle overdub |
| Fn + U | Stop and undo latest overdub |
| Fn + Backspace | Stop and clear loop |
| Fn + S / Fn + L | Save/load the selected preset slot |
| Fn + K | Learn key root from the next MIDI note |
| Fn + G | Learn an arbitrary MIDI control |
| Fn + Esc | Panic |

Bindings are centralized in `src/controller/KeyBindings.h` with a compile-time uniqueness check. Keys fire on press edges, not whenever another held key changes. Fn shortcuts never also fire their base key. Harmonic releases retain their original action even if Fn or the menu state changes. Menu commands run only on Enter, never from adjustment arrows. Performance shortcuts are suppressed while Options is open; panic remains available. Physical keyboard rollover still needs device testing.

Panic sends sustain-off, All Sound Off, Reset All Controllers, and All Notes Off on all 16 channels, explicitly releases every tracked pitch, cancels the scheduler, and clears every active voice. USB or BLE removal invokes the same path.

## MIDI defaults

- First boot: bypass
- Performance: block, channel 1, enabled
- Bass: off, channel 2
- Raw chord: off, channel 3
- Chord: major, no extensions, voicing 0
- Play style: Latched; source velocity retained
- Extensions: momentary; hold multiple extension pads to combine them
- Key: C major
- Tempo: 100 BPM, 1/16, 75% gate
- Expression: all enabled generated channels
- Input: omni, notes 0–127

Settings are versioned, checksummed, validated, and saved to NVS after a 1.5-second debounce when notes are released, the loop is stopped, and the DIN queue is empty. Give the device this idle interval before switching it off. Legacy settings are migrated on load. A malformed record leaves safe defaults.

## Display feedback

The 240×135 interface uses black, phosphor green and restrained amber, with bundled Orbitron display lettering and terminal labels. A large played-chord readout dominates the performance page; there is no instructional prose. The upper rail contains mode, output lane, receive-activity dot, USB, BT and SD. Scale status below the chord reads `AUTO` for automatic KEY harmony, `MANUAL` for an unsnapped manual KEY chord, `SNAP ON/OFF` for harmonic quantization, or `DIRECT` in BYPASS. `H` still toggles harmonic quantization.

The center shows a quiet dash at idle, the played harmony while a root is held, and a dimming 1.2-second history after release. Long chord names reduce their display size or separate the large root from the suffix. Panic clears history immediately. The named-note page describes the chord; loop-only playback shows `LOOP` instead of inventing a live chord.

A single spaced modifier rail shows `DIM MIN MAJ SUS 6 m7 M7 9` on chord, notes, keyboard, and diagnostics pages, without duplicated shortcut labels:

- Green: held quality button or active momentary extension.
- Amber: selected quality or toggled extension.
- Bright underline: the governing quality or active extension; a small corner mark also identifies a held pad.
- Dimmed labels: modifiers disabled in BYPASS.

These indicators update without playing a root. MIDI pads and Cardputer keys use the same state. KEY mode leaves quality buttons unselected until a manual quality is held. Modifier presses do not manufacture a chord preview or prolong old chord history. The footer contains performance/tempo, `HOLD` or `STACK`, playstyle, and loop state. Parameter feedback briefly replaces only the footer, leaving the chord and modifiers visible. The receive dot pulses independently of connection status.

Tab Options keeps its controls and contextual help, with a numbered progress rule, a large selected value, more space, and a navigation legend. Learning screens name the requested pad and show progress. Existing bindings are unchanged.

The keyboard page is live output, not a cached chord diagram. It follows messages accepted by the DIN sink after scheduler and note-ownership processing: green keys indicate Note On gates, and 50 ms amber marks indicate recent attacks, including notes whose gates are shorter than a frame. Arp/pattern advance note by note, strum/slop build in dispatch order, and harp releases individual gates. MIDI Note Off removes the gate even if the receiving synth sustains the sound. Panic clears gates and attack marks.

KEY/CHORD show the selected performance MIDI channel; bass/raw-chord streams on other channels do not obscure arp movement. Loop output on the selected channel is included. BYPASS displays all output channels. The octave-labelled window expands for multi-octave modes. The keyboard requests updates at up to 60 Hz, other views at 30 Hz; no MIDI event waits for an animation. Events closer together than a frame can appear together, and queued DIN acceptance is not a physical wire/audio timestamp. Runtime latency and frame rate require hardware measurement.

KEY mode builds its base triad from the scale, then adds the explicitly selected interval: 6 = nine semitones, m7 = ten, M7 = eleven, and 9 = fourteen above the chord root. In C Major, the additions are A, B-flat, B, and D respectively. These additions can leave the selected scale; m7 and M7 remain independent. CHORD mode still snaps the completed chord to the scale when harmonic quantization is enabled, so snapping can merge extensions there.

## Options and playstyles

Tab exposes key/scale, transpose, voicing, playstyle, extension behavior, performance mode/direction/rate/gate/strum/slop, BPM/clock, three output channels and stream enables, bass mode/octave, expression routing, velocity sensitivity, input channel/range, loop length/grid, and numbered storage slots. Below these parameters are learning, mapping, profile, preset, loop, and diagnostic commands.

Navigate with `;` / `.`, adjust with `,` / `/`, and use Enter for commands. Do not hold Fn for menu arrows. Tab closes Options; Esc/backtick backs out or cancels learning. Changes apply immediately; save commands wait until it is safe to access storage.

| Options setting | Values / use |
|---|---|
| Mode / Play style | BYPASS, CHORD, KEY / four playstyles below |
| Extensions / Extension addition | MOMENTARY or STACK / ADD NOTE or RETRIGGER |
| Key root / Scale | C–B / 12 scales listed above |
| Transpose / Voicing | −24…+24 semitones / −8…+8 voicing steps |
| Performance / Direction | Eight performance modes / UP, DOWN, UP/DOWN, RANDOM |
| Rate / Gate % | 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32 / 1–100% |
| Strum ms / Slop % | 2–120 ms between strum notes / 0–100% slop amount |
| BPM / Clock out | 30–300 BPM / outgoing MIDI clock ON or OFF |
| Performance ch / Bass ch / Raw chord ch | Independent MIDI output channels 1–16 |
| Performance on / Bass on / Raw chord on | Enable each stream independently; raw chord bypasses the rhythmic performance mode |
| Bass mode / Bass octave | OFF, ROOT, LOWEST, UNISON / −2…+1 octaves |
| Expression | SOURCE channel, GENERATED channels, or OFF |
| Input ch (0=all) / Lowest / Highest input | Omni or channel 1–16 / root-note range 0–127 |
| Loop bars / Loop grid | FREE or 1/2/4/8/16 bars / OFF or one of the six note divisions |
| Preset / loop slot | 1–16, shared slot selector for separate preset and loop files |
| Harmonic quantize | CHORD scale snapping ON/OFF; KEY automatic harmony is separate |
| Velocity sense | SOURCE VELOCITY or FIXED 100 |

The remaining command rows are Learn key root, Learn control, Pad setup, Mapping/delete, Select/Reload/Save profile, Save/Load preset, Loop record/play/overdub/undo/clear, Save/Load loop, Export diagnostics and Edit mapping. They require Enter; merely navigating to a command does not execute it.

- `LATCHED`: the existing workflow. Tap a chord quality, then play roots; quality stays selected. Extensions are still momentary unless `X` enables stacking.
- `SIMPLE`: hold a quality before pressing a root. Otherwise the root plays as a single note. The held chord keeps its original quality/extensions until root release.
- `ADVANCED`: start with a single note and press a quality to turn it into a chord. Its quality stays fixed until root release; extensions can change.
- `FREE`: start with a single note, add a quality, then switch/retrigger qualities while the root is held.

`Extension addition` selects common-tone preservation (`ADD NOTE`) or full retrigger. Timed performance modes restart their scheduling when harmony changes. In Key mode, holding a quality provides a manual override; releasing it restores diatonic harmony where the playstyle permits live changes. Fn+K consumes the next note and its release as key selection. Fn+Esc cancels key learning.

## Controller profiles

Profiles live in `/midi-brain/controllers/`; the default file is `smk37.json`. Options can select, reload, or save profiles. The selected path is remembered. On connection, matching prefers VID/PID, then manufacturer+product text, then product text; otherwise it uses the selected profile. USB strings and BLE advertised names are read for matching. One shared profile is active; use one input controller at a time.

Missing SD media or a missing/invalid profile produces an empty Generic mapping without disabling the musical engine. The parser rejects files over 64 KB, more than 128 mappings, malformed JSON, invalid bounds/types, and unknown actions. Up to 32 profile filenames are listed. Loading a profile also applies its input channel/range; use Save Profile to retain changes to those fields in the profile.

Example mapping:

```json
{
  "schema": 1,
  "id": "generic",
  "name": "Generic Controller",
  "input": {
    "channel": "omni",
    "root_note_low": 0,
    "root_note_high": 127
  },
  "mappings": [
    {
      "source": {"type": "cc", "channel": 1, "number": 21},
      "trigger": "relative",
      "relative_mode": "twos_complement",
      "action": "voicing.delta",
      "consume": true
    }
  ]
}
```

JSON channels are 1–16 or `"omni"`. Internal channels are 0–15. Supported inputs are note, CC, program change, channel pressure, and pitch bend. Supported triggers are press, release, press/release, toggle, absolute value, and relative value with two's-complement, binary-offset, or signed-bit decoding.

## Nine-pad setup

Press `O`, then press nine SMK pads in this order. Wait for the displayed step to advance before pressing the next pad:

```text
DIM  MIN  MAJ  SUS
 6   m7   M7    9
       LAYER NEXT
```

The wizard learns the actual message, channel, and cable. It requires release of the captured pad, a 350 ms guard, and a different selector before advancing. Use momentary note or CC pads, not latching pads that omit release. After the ninth pad, saving is deferred to the idle storage service; it writes/flushes a temporary file, retains a backup, renames it to `smk37.json`, and activates the mappings only on success. Learned events are consumed and do not sound downstream. A save failure is shown on screen.

The extension pads are momentary by default. Press `X` to enable stacking; extension presses then toggle persistent additions. Pressing `X` again returns to momentary operation. Switching clears the extension set; panic clears momentary extensions. Only stacked extensions persist across reboot. The performance footer shows `HOLD` or `STACK`.

For a receiver such as Ambient Zero configured with layers 1–4 on MIDI channels 1–4, `L` and the learned `LAYER NEXT` pad safely panic active notes, then cycle the primary output through those channels. Match the receiver's channel configuration. The selected layer appears as `L1`–`L4` in the upper rail and is saved in internal settings. In bypass mode, channel data remains transparent until lane selection is first used; after that, channel messages are sent to the selected layer.

Fn+G or Options → Learn Control captures a note, CC, program change, channel pressure, or pitch bend. Choose action, trigger, relative encoding, and whether it consumes MIDI; select Save Mapping and press Enter. Options → Edit Mapping edits the selected mapping; Mapping/Delete removes it. Left/right select the mapping index. New/edited mappings queue a profile save; deletions require Save Profile. Source number/channel/cable are captured exactly. To change a selector, delete it and learn again. Advanced ranges/match metadata can be edited in SD JSON, then reloaded from Options.

## MIDI loops and presets

Choose free length or 1/2/4/8/16 bars, then press Z before playing. Fixed-length recording starts immediately and begins playback at its boundary. In free mode, press Z or Space to close the recording. Space stops playback; Fn+Z enables/disables overdub; Fn+U stops and removes the latest overdub layer; Fn+Backspace clears. Recording again replaces the in-memory loop. There is no count-in or audio recording.

Loops store post-engine channel MIDI, including enabled performance/bass/raw streams, with original output channels. Playback does not get transformed a second time. Recording uses 96 ticks per quarter and optional 1/4, 1/8, 1/8T, 1/16, 1/16T, or 1/32 onset quantization. Note durations are retained and clipped at the cycle boundary. Capacity is 512 note/control entries and 128 pending notes; overflow is counted. Free length is bounded to 1024 bars. Loops retain their recorded BPM; changing the live BPM does not time-stretch an existing loop. Stop or re-record when changing tempo.

The screen shows loop state and entry count. Loop playback has separate note ownership; stopping it does not release a live-held copy of the same pitch. Loop sustain is reset on loop-used sustain channels at stop/wrap; put live sustained playing on a separate channel if independent pedal behavior is needed. Panic stops all playback.

Options offers Save Loop/Load Loop and 16 shared numbered preset/loop slots. Fn+S/Fn+L save/load controller settings, not loops. Files are stored as `/midi-brain/presets/preset-01.json` and `/midi-brain/loops/loop-01.json` through slot 16. Save targets are captured when requested; storage waits for released notes, stopped playback, and an empty DIN queue. Only one session-storage request can be pending. Loads stop active notes first. Invalid schemas, checksums, ranges, or unavailable SD fail visibly. Temporary files and `.bak` files protect replacement; real power-loss recovery still needs physical testing.

Settings auto-save to internal NVS without SD. Loops require explicit SD save and load and never auto-play after boot. Missing SD disables only SD-dependent operations.

## Diagnostics

Options → Export Diagnostics writes `/midi-brain/logs/diagnostics.json`: transport identity, counts, scheduler lateness, DIN high water, loop drops, and a 64-sample/one-second runtime history. Collection uses a fixed RAM ring; file output is deferred until idle. Adjacent expression messages are coalesced without crossing note events; Note Ons can displace coalescible traffic. If an exhausted DIN queue cannot accept a critical release, the next main-loop pass clears queued traffic and issues a full panic. BLE queue overflow discards the incomplete input batch and invokes panic. These paths are safety recovery, not a claim of lossless MIDI under unlimited load.

## Architecture

The engine under `src/app`, `src/engine`, `src/midi`, `src/controller`, and `src/scheduler` is ordinary allocation-free C++ in the performance path. Hardware dependencies stay under `src/transport`, `src/hardware`, `src/storage`, and `src/ui`.

The native ESP-IDF USB Host Library scans the active configuration, bounds-checks descriptors, and selects a claimable MIDIStreaming interface/alternate setting with an input endpoint. It ignores Audio Streaming interfaces, accepts bulk or interrupt endpoints, and does not require a hardcoded VID/PID. It does not switch USB configurations or support external hubs. MIDI arrives as normalized `MidiEvent` values. Every scheduled note carries a `VoiceId` and stream owner. Physical channel/pitch reference counting protects overlapping live and loop voices.

## Known limitations

- SMK-37 Bluetooth can connect without key input; switch the keyboard off/on twice. A fix is planned for a coming version, not included in 1.0.
- Cardputer 1.0 compatibility was reported working; 1.1 physical testing and exhaustive per-release hardware/latency checks remain outstanding.
- One USB device and the first claimable MIDIStreaming interface are supported at a time.
- BLE MIDI supports one automatically discovered standard BLE-MIDI peripheral at a time and has no on-device chooser.
- USB is host input, not a USB-MIDI device output to a DAW. BLE is controller input only.
- No SysEx forwarding, external-clock-follow mode, ORC secret-chord tables, or factory-pattern library.
- Loops retain recorded tempo/channels, and notes crossing a loop boundary are clipped. There is no count-in or file naming UI.
- No synth, audio, effects, drum sounds, or waveform display.
- The scheduler is deterministic and bounded, but sustained DIN saturation needs physical stress measurement.

See [docs/HARDWARE.md](docs/HARDWARE.md), [docs/MIDI_BEHAVIOR.md](docs/MIDI_BEHAVIOR.md), [docs/USB_DEBUG.md](docs/USB_DEBUG.md), and [docs/TEST_PLAN.md](docs/TEST_PLAN.md) before hardware testing.

## Troubleshooting

| Symptom | Check |
|---|---|
| BT green, no RX from SMK-37 | Switch the SMK-37 off/on twice with M5Chord running. Disconnect other BLE hosts. Verify `N` and `E` rise when playing. |
| No USB detection | Use a data cable, direct OTG connection and the controller's USB-MIDI mode. External hubs, including powered hubs, are unsupported. Use BLE for SMK-37. |
| Notes received, no sound | Unit MIDI must be in SEPARATE; connect DIN OUT to receiver IN. Check receiver channel, BYPASS, Performance on and stream routing. M5Chord generates no audio. |
| CHORD plays a single note | Select LATCHED or hold a quality in another playstyle. Confirm CHORD, not BYPASS, and Performance on. |
| Qualities will not stack | DIM/MIN/MAJ/SUS choose one quality. Stack applies to 6/m7/M7/9, toggled by `X`. |
| Extensions seem identical | Check Harmonic quantize in CHORD: snapping can merge notes. Turn it off to hear literal intervals. |
| Pads sound notes instead of controlling | Complete Pad setup or Learn Control, save the profile, and enable Consume for those mappings. |
| Pad setup does not save | Insert a writable FAT32 microSD card. Release pads and stop playback so deferred storage can finish. |
| Settings disappear | Allow at least 1.5 seconds idle after changes. Factory binary installation resets NVS; use separate-component updates. SD presets/loops require explicit save. |
| Arp appears frozen | Hold a root; select ARP and enable the performance stream. Keyboard view follows output gates, not just the selected chord. |
| Stuck notes | Fn + Esc/backtick sends panic. Check controller releases/pedal, receiver channels and disconnect behavior. |
| Flickering or no screen | Verify power, flash address and complete image. Release G0, power-cycle, and report the last visible boot stage. |

For a bug report, include Cardputer revision, M5Chord version, controller model/firmware, BLE versus USB, power/cable/hub setup, SD presence, receiver and MIDI channel, exact steps, and Geek counters. Attach diagnostics if available; review them before posting because they contain device/profile identity strings.

## License and credits

M5Chord source is MIT-licensed; see [LICENSE](LICENSE). Dependencies and bundled font data retain their own licenses; see [third-party notices](THIRD_PARTY_NOTICES.md). Thanks to M5Stack, Espressif, Arduino, the library authors and font designers. Controller inspiration and remaining differences are recorded in the [ORC-1 assessment](docs/ORC1_CONTROLLER_ASSESSMENT.md); M5Chord does not include synth features.
