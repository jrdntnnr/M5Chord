# M5Chord

Version: **1.1.0**. Versioning uses `MAJOR.MINOR.PATCH`: new compatible features increase MINOR, fixes alone increase PATCH, and breaking changes increase MAJOR. The previous [1.0 release](https://github.com/jrdntnnr/M5Chord/releases/tag/v1.0) remains available.

New in 1.1.0: standard BLE-MIDI discovery, handshake changes and readable logs, Unit MIDI DIN input, explicit input selection, seven shortcut-help pages, a saved layer-channel count, and SD MIDI-file playback. See the [1.1.0 release notes](docs/RELEASE_1.1.0.md). The BLE repair remains a candidate until repeated physical cold-start tests pass.

M5Chord is a standalone harmonic MIDI controller for M5Stack Cardputer 1.0, 1.1 and ADV. Use a MIDI keyboard, pad controller, sequencer or other MIDI source over USB, Bluetooth LE or DIN. Turn notes into chords, add extensions, play arpeggios and strums, and send the result to any receiver that accepts the generated MIDI messages over DIN MIDI.

M5Chord generates **MIDI, not audio**. An M5Stack Unit MIDI in `SEPARATE` mode provides the DIN input/output interface; the firmware does not add a synth engine or controls for the Unit's onboard synthesizer. No computer is needed while playing.

[Download M5Chord 1.1.0](https://github.com/jrdntnnr/M5Chord/releases/tag/v1.1.0) · [Report an issue](https://github.com/jrdntnnr/M5Chord/issues) · [Release notes](docs/RELEASE_1.1.0.md)

> **Compatibility:** M5Chord targets standard MIDI 1.0, not a particular controller or instrument. USB power/descriptor requirements and BLE discovery requirements still apply; see [supported connections](#supported-connections) and [device-specific test notes](#device-specific-test-notes).

## Contents

- [SD MIDI-file playback](#sd-midi-file-playback)
- [Supported connections](#supported-connections), [signal path](#signal-path), and [hardware/wiring](#hardware)
- [Install the 1.1.0 binary](#install-the-110-binary)
- [Quick start: chords and arp](#quick-start-chords-and-arp)
- [USB connection](#first-connection) and [Bluetooth connection](#ble-midi-connection)
- [DIN MIDI input](#din-midi-input) and [input selection](#input-selection)
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
| ADV | TCA8418 keyboard | Latest diagnostic build approved by user; exhaustive release checks remain pending |

Release 1.1.0 retains the shared compatibility code and adds the features described above. Do not interpret build success as exhaustive hardware validation. See [compatibility details](docs/COMPATIBILITY.md) and [recorded results](docs/SMK37_TEST_RESULTS.md). This project is independent of M5Stack and is not an official M5Stack product.

## Supported connections

| Connection | Direction | Requirements |
|---|---|---|
| USB MIDI | Input | A directly connected, class-compliant USB-MIDI 1.0 device; suitable power and a data/OTG connection. No external hubs or vendor-specific drivers. |
| Bluetooth LE MIDI | Input | A standard BLE-MIDI peripheral discoverable as described below. Not Bluetooth audio, Bluetooth Classic or a HID keyboard. |
| DIN MIDI | Input and output | Unit MIDI connected to Grove; source MIDI OUT to Unit INPUT, and Unit OUTPUT to receiver MIDI IN. Use SEPARATE for processed output. |
| SD MIDI file | Playback source | Supported Standard MIDI Files in `/midi`; playback goes to DIN output. |

There is no controller-brand whitelist. Compatibility means using the supported transport and MIDI messages, not a guarantee that every product with a MIDI or Bluetooth label will work. Select one live input at a time in Tab → MIDI input. USB and BLE are input-only in this firmware; they do not send generated MIDI back to a computer or wireless receiver.

## Signal path

```text
USB-MIDI source ── USB host ──────────────────┐
BLE-MIDI source ── Bluetooth LE ──────────────┤
DIN MIDI source ─ Unit MIDI INPUT ─ GPIO1/RX ─┤
                                              ▼
                      M5Chord / Cardputer 1.0, 1.1 or ADV
                              One selected live input
                                              │
                                      Grove GPIO2/TX
                                              ▼
                                Unit MIDI [SEPARATE]
                                              │
                                       DIN MIDI OUT
                                              ▼
                                External MIDI receiver
```

The DIN input and output above use the same Unit MIDI. On the Cardputer's Grove connector, **GPIO1 is MIDI RX** and **GPIO2 is MIDI TX**. SD MIDI-file playback and recorded loops also feed DIN output; file playback is separate from live input selection.

## Hardware

- M5Stack Cardputer 1.0, 1.1 or ADV
- M5Stack Unit MIDI with its front switch set to `SEPARATE`
- A MIDI source using standard BLE-MIDI, class-compliant USB MIDI, or DIN MIDI; not required for SD-file playback
- DIN-MIDI receiver, synth, effects processor, or other instrument
- The data/MIDI cables required by your chosen connection; use a proper data/OTG path for USB input
- Optional FAT32 microSD card for controller profiles, presets, loops, MIDI files and diagnostics

The Grove connection is GND to GND, 5 V to 5 V, Cardputer GPIO2/TX to Unit UART_RX, and Cardputer GPIO1/RX to Unit UART_TX. The firmware configures UART2 for 31,250 baud, 8N1, RX GPIO1, and TX GPIO2.

USB MIDI input is intended for **low-powered, simple, class-compliant MIDI controllers connected directly**. Compatibility also depends on USB descriptors and a proper data/OTG connection; low power consumption alone is not a guarantee. Higher-powered or more complex devices may not enumerate reliably. External USB hubs, including powered hubs, are unsupported; use a compatible BLE or DIN connection instead. Device-specific results are listed in the test notes below.

## SD folders

For a fresh installation, boot with a writable FAT32 microSD card. M5Chord creates **`/M5Chord`** and its `controllers`, `presets`, `loops` and `logs` subfolders automatically. No existing user's files are required. Controller mappings, presets, recorded loops and diagnostic exports use these folders.

Playback files remain separate: copy `.mid`/`.midi` files into **`/midi`** at the SD root. MIDI Player → Load creates that folder if it does not exist.

Upgrading from the old folder layout: with the device powered off and before running this updated firmware, manually rename **`midi-brain` → `M5Chord`**, keeping everything inside it. There is no automatic move or fallback to the old SD folder. If `/M5Chord` already exists, back up both folders before merging their contents; do not overwrite newer files blindly. A previously selected custom profile may need to be selected again under Tab → Select profile; the legacy default pad-profile filename remains `controllers/smk37.json` in 1.1.0; its name does not restrict which controller can be learned. Internal settings storage is unchanged, so the SD rename does not reset saved settings or Layer channels. Older firmware still expects `/midi-brain`; pair the folder rename with the updated firmware.

## Input selection

Tab → **MIDI input** selects AUTO, BLE, USB or DIN. AUTO locks to the first source delivering a channel MIDI message; timing-only traffic does not claim it. A disconnect/error from the selected BLE/USB source releases its notes; a disconnect from an unused source does not cut the active performance. Change MIDI input to choose another source; changing it panics active notes and resets the selection. The selection is saved.

M5Chord deliberately uses one input source at a time. It is not a multi-controller merger: identical channel/note messages from two controllers must not release one another's notes. An ordinary DIN cable provides no connection-detection signal. If a DIN device sends MIDI Active Sensing, missing traffic for more than 300 ms triggers cleanup; otherwise use Fn + Esc if it is unplugged while a note is held.

## DIN MIDI input

Connect your controller or sequencer's DIN OUT to the **Unit MIDI INPUT**, and the Unit OUTPUT to your receiver's MIDI IN. Keep the Unit switch at **SEPARATE** for transformed output; BYPASS physically connects input to output and bypasses the chord engine on that wire. Select Tab → MIDI input → DIN, then inspect Geek view's `DIN E` counter while playing.

M5Chord reads the Unit's optoisolated input through Grove **UART_TX → Cardputer GPIO1/RX** at 31,250 baud, 8N1; GPIO2/TX still carries generated output. This is MIDI input through the Unit MIDI hardware, **not audio input and not a SAM2695 synthesizer API**. No synth setup or audio feature is added.

The [Unit MIDI schematic](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/774/SCH_UnitMIDI_B04_sch_2024_07_08_15_41_29.pdf) shows the optoisolated input path to Grove. The [product page](https://docs.m5stack.com/en/unit/Unit-MIDI) gives conflicting wording for input in SEPARATE mode, so the documented wiring is based on the schematic; firmware DIN input is implemented, while per-revision hardware acceptance remains recorded separately. Do not loop the receiver's MIDI THRU/OUT back into this input.

The parser handles running status, Note On velocity zero, channel messages, interleaved realtime and song position. SysEx/system-common messages outside the supported set are skipped safely. UART overflow/framing errors reset parsing and panic the selected source. It does not infer disconnection from a quiet keyboard that does not send Active Sensing.

## Install the 1.1.0 binary

Download and extract `M5Chord-v1.1.0.zip` from the [1.1.0 release](https://github.com/jrdntnnr/M5Chord/releases/tag/v1.1.0). It contains the universal image, separate update components, checksums, instructions and third-party notices.

| File | Address | Purpose |
|---|---|---|
| `M5Chord-v1.1.0-universal.bin` | `0x0000` | Fresh installation on any supported Cardputer; resets internal settings |
| `M5Chord-v1.1.0-app.bin` | `0x10000` | Application component; never flash this at zero |
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
python -m esptool --chip esp32s3 --port PORT --baud 460800 write_flash 0x0000 M5Chord-v1.1.0-universal.bin
```

### Update while retaining M5Chord settings

Use this instead of the merged image when updating an existing build of this project:

```sh
python -m esptool --chip esp32s3 --port PORT --baud 460800 write_flash 0x0000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 M5Chord-v1.1.0-app.bin
```

This selects app0 and writes only the programming regions, leaving NVS untouched. App-only flashing is for advanced users who have verified the same `default_8MB` partition layout and active app0 slot. Do not use erase-all or change the flash settings supplied by the binary.

Expect writing progress and `Hash of data verified.` On completion, release G0 and power-cycle the Cardputer. You should see **M5Chord / V1.1.0** during startup, followed by the performance screen. Fresh settings start in CHORD and Keyboard view; saved mode/view preferences take precedence. Press Tab to open Options, and `V` to cycle to Geek view: `1.0/1.1` or `ADV` identifies the keyboard family.

The programming port normally disappears when the app switches native USB into host mode. It is not a USB-MIDI output or serial console while playing. If upload fails, check the data cable, re-enter G0 download mode and retry at `--baud 115200`.

Verify downloads with `shasum -a 256 -c SHA256SUMS` on macOS or `sha256sum -c SHA256SUMS` on Linux. On Windows, compare `Get-FileHash FILE -Algorithm SHA256` with the corresponding checksum. Flash-address behavior follows the [official esptool instructions](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-commands.html).

M5Burner catalog publication has not been performed. The merged image is packaged for installation at zero, but do not assume the project is already listed in M5Burner.

## Quick start: chords and arp

1. Connect Unit MIDI in `SEPARATE` mode and its DIN OUT to your instrument's MIDI IN. Set the receiver to channel 1 initially.
2. Connect a compatible MIDI source over BLE, direct USB or Unit MIDI DIN INPUT. Select the corresponding Tab → MIDI input setting, or AUTO. Check BYPASS first: notes should reach the receiver, and the receive dot/counters should react.
3. Press `M` until CHORD. Open Tab Options, set **Play style = LATCHED**, **Performance = BLOCK**, **Performance on = ON**, **Output channel = 1**, and **Harmonic quantize = OFF**. Close Tab.
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

The generated application image is `.pio/build/cardputer-universal/firmware.bin` at offset `0x10000`. The legacy `cardputer-adv` environment remains a build alias with the same shared configuration. Do not flash the application at zero. Clone the source with `git clone https://github.com/jrdntnnr/M5Chord.git`, enter `M5Chord`, and install PlatformIO Core before building. To reproduce this release, select tag `v1.1.0` (the older release remains at `v1.0`).

To package a release, use the Python interpreter running PlatformIO with its esptool dependencies available:

```sh
python tools/package_firmware.py --output dist/M5Chord-current
python -m unittest discover -s tools -p 'test_package_firmware.py'
```

Use `--core-dir` if PlatformIO's packages are not in `.pio`. The package script derives the version from AppInfo, accepts development/RC versions, requires committed tracked source and no untracked files, rejects diagnostic firmware, and requires a new output directory. It never reads flash from a connected device. The published v1.0 tag retains its original packaging instructions.

## Desktop tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Install the pinned PlatformIO dependencies first; host JSON tests use the same ArduinoJson headers. An alternative location can be supplied with `-DARDUINOJSON_INCLUDE=/absolute/path/to/ArduinoJson/src`.

Tests cover harmony, transport parsing, scheduling, note ownership, panic, exact-modifier keyboard edges, menu confirmation, MIDI Learn/edit/delete, playstyles, overlapping extension controls, profile/preset/loop validation, recording/overdub/undo, shared live/loop notes, diagnostic ring wrap, and DIN congestion recovery.

## First connection

1. Power the MIDI source and enable its MIDI output mode if required.
2. Connect Unit MIDI to Grove, set its switch to `SEPARATE`, and connect Unit OUTPUT to the receiver's MIDI IN.
3. Boot the Cardputer and choose one input path: direct USB MIDI, BLE MIDI, or the source's DIN OUT into Unit INPUT.
4. In Tab → MIDI input, select USB, BLE or DIN explicitly for a first test, or use AUTO. In AUTO, the first channel message selects the active source; other sources are ignored until that source disconnects or you change the setting.
5. Play and release a note. The receive dot and Geek-view `RX` counter should react. For USB, green `USB` means the interface is connected, not that notes are arriving. For BLE, inspect `N`/`E`; for DIN, inspect `DIN E`.
6. Select M5Chord's `BYPASS` mode for the initial passthrough test; leave the **Unit's physical switch at SEPARATE**. Match the receiver's MIDI channel to the source, or use an omni receiver setting. Check notes, releases and any expression messages you use.
7. Select `CHORD`, choose a quality and play a root. Test note release before enabling rhythmic performance modes. Fresh settings use CHORD with the Latched playstyle; existing saved preferences may differ.

For USB, repeated resets or attachment loops can indicate a power, cable or descriptor-compatibility problem. The pinned host stack does not support external hubs. A separately powered device still needs a direct USB data path; a powered hub does not make that path supported.

## BLE MIDI connection

Controllers from any brand are candidates when they expose the **standard BLE-MIDI service and notification characteristic** and meet the discovery requirements. The scanner accepts an advertised MIDI service UUID or a case-insensitive name containing `MIDI`; a legacy fallback also recognizes the tested keyboard named in the device-specific notes. GATT discovery then validates the service and characteristic before subscription. Devices advertising neither the UUID nor a recognized name are not automatically found.

Bluetooth Classic, audio/A2DP, HID-only keyboards and proprietary non-MIDI protocols are not supported. M5Chord connects to one BLE peripheral at a time, using the first queued candidate that passes setup; there is no manual device-picker UI. Turn off other nearby MIDI peripherals if you want to control which is found.

1. Disconnect any computer, phone or tablet already using the controller's BLE-MIDI connection.
2. Enable the controller's **Bluetooth MIDI** mode—not an audio or HID mode.
3. Boot the Cardputer. Scanning and connection are automatic; M5Chord has no passkey-entry UI.
4. `BT` is amber while scanning, setting up, or connected without decoded MIDI in this session. It turns green after a MIDI event is decoded; play a note to confirm input.
5. Press `V` to reach Geek view. `BT SUB` means subscription setup; `BT ON` means subscription accepted. `N` counts received notifications and `E` counts decoded MIDI events. The receive dot and `RX` confirm messages accepted by the selected input path; a BLE connection alone does not select BLE when another input is active.

Tab → **BLE reconnect** releases active notes and requests a fresh connection; connection work waits until the engine and DIN queue are idle. It does not guarantee that a controller will resume advertising without being restarted.

Version 1.1.0 performs an acknowledged initial MIDI characteristic read, notification setup and a guarded MTU-exchange request. Authentication-required reads have a bounded encryption retry, but devices requiring interactive passkey entry are not supported. There is no idle reconnect watchdog: silence may simply mean nobody is playing. USB, BLE and DIN use the same controller mapping, musical engine, scheduler and note-ownership path after input selection.

## Device-specific test notes

These devices were used for testing; they are **not required hardware** and do not define a controller or receiver whitelist.

- **EasyPlay1 Plus:** reported working as a directly connected USB-MIDI input in its MIDI mode. A powered USB hub did not work; hubs remain unsupported.
- **SMK-37:** used for BLE-MIDI testing; one observed advertised name was `SMK-37 Pro_BLE`. It can connect without registering keys. The reported workaround is to switch the keyboard off and back on twice while M5Chord stays running. Cardputer-side reconnect and an older subscription toggle did not resolve the tested first-connection failure. The 1.1.0 read/MTU changes are not a verified fix. Direct USB interoperability has not been established.

Detailed setup, image hashes and limitations of the observations are in [physical test results](docs/SMK37_TEST_RESULTS.md). Results from one controller, receiver or firmware build are not blanket certification of other devices.

## BLE logging

Normal firmware keeps a bounded 128-record BLE trace in RAM. Tab → Export diagnostics saves it inside `/M5Chord/logs/diagnostics.json` when notes are released, loop/file playback is stopped and DIN output is drained. The trace includes monotonic microseconds, scan/connect/discovery, handles, initial read/results, registration, descriptor writes, MTU, connection parameters, security/authentication and disconnect reasons. The first 16 accepted notifications and up to 16 rejected notifications per session are sampled, with at most 16 payload bytes each. MIDI handling does not write files or render UI. Trace queue drops are counted; the USB console also reports free heap and main-task stack high-water margin.

For live diagnosis over a USB cable to a computer, build **cardputer-diagnostics**. It keeps BLE and DIN available but **disables USB-MIDI host input**, because the same native USB peripheral is used as the console:

```sh
PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-diagnostics
PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-diagnostics -t upload --upload-port PORT
python tools/read_ble_log.py --port PORT --seconds 60 --output .local/ble-startup.log
```

Use the PlatformIO Python interpreter or install `pyserial` in a virtual environment. The capture script refuses to overwrite an existing log. It opens the serial port without an intentional reset and requests the buffered trace. `--reconnect` requests a BLE reconnect for another trial. In a serial terminal, `l` replays the trace, `r` requests reconnect, and `p` invokes panic. Console output is bounded and nonblocking; a disconnected computer must not hold up MIDI.

To test: leave Cardputer USB attached, disconnect other BLE hosts, start the logger, turn the MIDI controller on once and play notes. Look for `OPEN`, `DISCOVERY`, `READ_RESULT`, `REGISTER_RESULT`, `CCCD_ON`, `WRITE_RESULT`, `READY`, then `NOTIFY` and increasing `N`/`E`. `status=0` on GATT results means success; NOTIFY status reports the number of decoded events. `LINK`, `MTU_REQUEST` and `MTU` expose connection/MTU setup; a fallback request is only logged when needed. The old automatic CCCD off/on repair is no longer used. Save logs for both failing and working sessions, recording hardware and startup order. Review device names/MIDI payloads before sharing logs publicly.

After diagnosis, flash **cardputer-universal** to restore USB-MIDI host input. Do not distribute the diagnostics binary as the normal universal build.

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
| B | Toggle root bass on the configured bass channel (default 2) |
| T | Tap tempo; four plausible taps establish BPM |
| C | Toggle MIDI clock and send Start/Stop |
| H | Toggle manual harmonic quantization |
| V | Cycle chord, notes, keyboard, and Geek views |
| L | Cycle the primary output through channels 1–N; Tab → Layer channels sets N, default 4 |
| X | Toggle momentary / stack extension behavior and clear active extensions |
| O | Start the nine-pad setup wizard for your MIDI controller |
| Tab | Open/close Options |
| `;` / `.` | Previous/next Options row |
| `,` / `/` | Decrease/increase a parameter; select mapping index |
| Enter | Confirm the selected command |
| Esc/backtick | Open/close shortcut help; cancel an active learning/editor screen |
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

**Help:** Esc opens seven pages containing every bound shortcut. Tab → Help / shortcuts → Enter opens the same list. Use `;` / `,` for the previous page, `.` / `/` / Enter for the next, and Esc to return. From Options, Esc opens help and Esc returns to Options; Tab closes Options. From performance help, Tab opens Options. Fn + Esc remains panic everywhere. Releasing a held chord modifier while viewing help still releases that modifier.

Panic sends sustain-off, All Sound Off, Reset All Controllers, and All Notes Off on all 16 channels, explicitly releases every tracked pitch, cancels the scheduler, and clears every active voice. Disconnecting the selected USB/BLE input, or a detected fault on the selected DIN input, invokes the same path; disconnecting an unused source does not.

## MIDI defaults

- First boot: CHORD and Keyboard view in 1.1.0; existing saved mode/view choices take precedence.
- Performance: block, channel 1, enabled
- Bass: off, channel 2
- Raw chord: off, channel 3
- Chord: major, no extensions, voicing 0
- Play style: Latched; source velocity retained
- Extensions: momentary; hold multiple extension pads to combine them
- Key: C major
- Tempo: 100 BPM, 1/16, 75% gate
- Expression: all enabled generated channels
- Input source: AUTO; channel filter: omni; root-note range: 0–127
- Layer channels: 4; MIDI-file playback is stopped until explicitly started

Settings are versioned, checksummed, validated, and saved to NVS after a 1.5-second debounce when notes are released, loop/file playback is stopped, and the DIN queue is empty. Give the device this idle interval before switching it off. Legacy settings are migrated on load. A malformed record leaves safe defaults.

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

KEY/CHORD show the selected performance MIDI channel; bass/raw-chord streams on other channels do not obscure arp movement. Loop and MIDI-file output on the selected channel are included. BYPASS displays all output channels. The octave-labelled window expands for multi-octave modes. The keyboard requests updates at up to 60 Hz, other views at 30 Hz; no MIDI event waits for an animation. Events closer together than a frame can appear together, and queued DIN acceptance is not a physical wire/audio timestamp. Runtime latency and frame rate require hardware measurement.

KEY mode builds its base triad from the scale, then adds the explicitly selected interval: 6 = nine semitones, m7 = ten, M7 = eleven, and 9 = fourteen above the chord root. In C Major, the additions are A, B-flat, B, and D respectively. These additions can leave the selected scale; m7 and M7 remain independent. CHORD mode still snaps the completed chord to the scale when harmonic quantization is enabled, so snapping can merge extensions there.

## Options and playstyles

Tab exposes key/scale, transpose, voicing, playstyle, extension behavior, performance mode/direction/rate/gate/strum/slop, BPM/clock, three output channels and stream enables, bass mode/octave, expression routing, velocity sensitivity, input channel/range, loop length/grid, and numbered storage slots. Below these parameters are learning, mapping, profile, preset, loop, and diagnostic commands.

Navigate with `;` / `.`, adjust with `,` / `/`, and use Enter for commands. Do not hold Fn for menu arrows. Tab closes Options; Esc opens help, or cancels active learning/editing. Changes apply immediately; save commands wait until it is safe to access storage.

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
| Output channel / Bass ch / Raw chord ch | Independent MIDI output channels 1–16 |
| MIDI input | AUTO, BLE, USB or DIN; source isolation, not merging |
| Layer channels | 1–16, default 4; L and the mapped layer pad cycle channels 1 through this count |
| Performance on / Bass on / Raw chord on | Enable each stream independently; raw chord bypasses the rhythmic performance mode |
| Bass mode / Bass octave | OFF, ROOT, LOWEST, UNISON / −2…+1 octaves |
| Expression | SOURCE channel, GENERATED channels, or OFF |
| Input ch (0=all) / Lowest / Highest input | Omni or channel 1–16 / root-note range 0–127 |
| Loop bars / Loop grid | FREE or 1/2/4/8/16 bars / OFF or one of the six note divisions |
| Preset / loop slot | 1–16, shared slot selector for separate preset and loop files |
| Harmonic quantize | CHORD scale snapping ON/OFF; KEY automatic harmony is separate |
| Velocity sense | SOURCE VELOCITY or FIXED 100 |

The remaining command rows are Learn key root, Learn control, Pad setup, Mapping/delete, Select/Reload/Save profile, Save/Load preset, Loop record/play/overdub/undo/clear, Save/Load loop, Export diagnostics, Edit mapping, Help/shortcuts, BLE reconnect and MIDI Player. They require Enter; merely navigating to a command does not execute it.

- `LATCHED`: the existing workflow. Tap a chord quality, then play roots; quality stays selected. Extensions are still momentary unless `X` enables stacking.
- `SIMPLE`: hold a quality before pressing a root. Otherwise the root plays as a single note. The held chord keeps its original quality/extensions until root release.
- `ADVANCED`: start with a single note and press a quality to turn it into a chord. Its quality stays fixed until root release; extensions can change.
- `FREE`: start with a single note, add a quality, then switch/retrigger qualities while the root is held.

`Extension addition` selects common-tone preservation (`ADD NOTE`) or full retrigger. Timed performance modes restart their scheduling when harmony changes. In Key mode, holding a quality provides a manual override; releasing it restores diatonic harmony where the playstyle permits live changes. Fn+K consumes the next note and its release as key selection. Fn+Esc cancels key learning.

## Controller profiles

Profiles live in `/M5Chord/controllers/` and can describe any compatible MIDI controller. Version 1.1.0 retains the historical default filename `smk37.json` and wizard profile label `SMK37` from testing; neither is a device requirement or a built-in set of pad assignments. Options can select, reload, or save profiles. The selected path is remembered. On connection, matching prefers VID/PID, then manufacturer+product text, then product text; otherwise it uses the selected profile. USB strings and BLE advertised names are read for matching. One shared profile is active; use one input controller at a time.

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

Press `O`, then press nine distinct MIDI pads or momentary controls in this order. They may be on any supported USB, BLE or DIN controller. Wait for the displayed step to advance before pressing the next pad:

```text
DIM  MIN  MAJ  SUS
 6   m7   M7    9
       LAYER NEXT
```

The wizard learns the actual message, channel, and cable. It requires release of the captured pad, a 350 ms guard, and a different selector before advancing. Use note controls that send Note Off (or velocity-zero Note On), or CC controls that return to zero when released; do not use latching pads that omit release. If your controller has fewer than nine suitable controls, use Fn+G / Learn control to map individual actions instead. After the ninth pad, saving is deferred to the idle storage service; it writes/flushes a temporary file, retains a backup, renames it to the legacy `smk37.json` filename, and activates the mappings only on success. Learned events are consumed and do not sound downstream. A save failure is shown on screen.

The extension pads are momentary by default. Press `X` to enable stacking; extension presses then toggle persistent additions. Pressing `X` again returns to momentary operation. Switching clears the extension set; panic clears momentary extensions. Only stacked extensions persist across reboot. The performance footer shows `HOLD` or `STACK`.

Tab → **Layer channels** controls how many MIDI channels `L` and the learned `LAYER NEXT` pad cycle through: 1–16, default **4**. It also limits MIDI-file playback channels. For a receiver configured for four independent MIDI channels, leave it at four. Set eight to cycle 1→2→…→8→1; one always selects channel 1. The value is saved. Changing the count stops MIDI-file playback and releases its notes; it does not retune a live-held note or immediately change the selected live channel. The next L/pad press selects within the new range and panics old notes first. Match the receiver's configuration. The upper rail shows `L1`–`L16`. BYPASS retains input channels until lane/output-channel selection enables overriding them.

## SD MIDI-file playback

1. Put Standard MIDI Files (`.mid` or `.midi`, case-insensitive extension) in **`/midi` at the root of a FAT32 microSD card**. Use filenames shorter than 64 bytes. Insert the card before boot; use a computer/card reader to copy files. USB mass-storage mode is not provided.
2. Open **Tab → MIDI Player → Enter**. From the first Options row, one Up press reaches MIDI Player. The dedicated page has **Load** and **Play/Stop**, not three separate numbered commands. Choose a button with `,` / `/` (or `;` / `.`), then Enter. The Options number is a menu position, not a MIDI channel.
3. Choose **Load**, browse `/midi` with `;` / `.` (or `,` / `/`), then Enter. Browsing/loading stops musical playback and drains queued output first. The folder is created if absent on a writable card. Validation runs in the background with **CHECKING FILE %** and a progress bar; the interface remains usable. Leaving the page does not cancel validation. Invalid files leave a persistent explanation on the player page, not an expiring toast.
4. Successful loading selects **Play** if the page is still open; playback never starts automatically. Enter starts from the beginning. The button becomes **Stop**; Enter again stops, not pauses. The page shows the filename, elapsed/total time and used/available note-playing channels: **4/5 CH** means four of five channels are included. Esc returns to Options, Tab exits without stopping playback. Fn + Esc remains panic everywhere. No new physical performance shortcuts were added; `file.stop` remains available for MIDI mapping.
5. Adjust **Tab → Layer channels** before playing. Default is four. The file stays loaded after stopping, but is not restored after reboot. Loading another file replaces it; a failed load leaves no playable file.

Channel policy: choose the lowest-numbered source channels containing Note On events, keep at most N, and map them in ascending source-channel order to output channels **1–N**. If a file uses channels 2, 4, 6, 8 and 10 with N=4, they map to outputs 1, 2, 3 and 4; source channel 10 is omitted. Notes, velocity, expression, program changes and pitch bend follow the same mapping. This is a **channel limit, not a polyphony or track limit**: a chord on one channel retains all its notes, and multiple tracks on the same channel stay together. MIDI channel 10 has no special drum reservation; its mapping may change, so prepare files for the receiving instrument's channel layout.

Playback sends the file's MIDI directly to DIN output, without chord/scale/arp transformation. It follows the file's tempo map (120 BPM if absent), not the live BPM setting or external MIDI clock. Live clock output, if enabled, remains at the live BPM and is not synchronized to the file; disable it when unwanted. Playback is one-shot, with no seek, pause, repeat, tempo override or file editing in this version. Starting file playback stops the looper and releases live notes; starting loop record/play/overdub stops the file. Live playing can accompany a file afterward, but share channels deliberately: MIDI sustain/controllers affect the whole channel. File stop resets sustain, sostenuto and hold-2 on file output channels; program/controller changes otherwise remain on the receiver.

Supported: SMF format 0 and 1, PPQN timing, running status, tempo changes, multiple tracks, Note On/Off (including velocity-zero release), CC below 120, program change, pressure and pitch bend. Limits: **1 MiB file, 32 tracks, 256 simultaneous file-owned notes, 24-hour duration**, with bounded shared scheduler/output queues. The device no longer has a 2,048-event whole-song limit: it validates the complete file, then streams through a fixed **512-event read-ahead buffer**. Files exceeding parse limits are rejected, not partially played. Dense output may exceed DIN bandwidth; overload recovery prioritizes release safety. Unmatched notes are released at end/stop. SysEx and non-tempo metadata are skipped; CC 120–127 are suppressed to avoid file-triggered channel-wide resets. Format 2, SMPTE division, nonzero MIDI-port metadata, RIFF/RMID and MIDI 2.0 files are not supported. Export format 0/1 PPQN from your sequencer if needed. These limits make this a small controller-side player, not an unrestricted DAW file engine.

The browser lists up to 32 files, alphabetized after discovery, and examines at most 512 directory entries. Subfolders are not searched. A background storage task owns the open file and uses a fixed read cache; the musical engine only consumes buffered events. MIDI event processing does not access SD, allocate memory, or bypass `VoiceId`, the scheduler and active-note registry. **Keep the SD card inserted while playing.** An SD read error or buffer underrun stops playback and releases file-owned notes; the reason stays on the player page. Restarting playback rewinds and buffers again. The existing output-keyboard view shows dispatched file notes alongside live output. Physical SD playback, DIN receiver behavior and sustained-load margins remain acceptance checks in [TEST_PLAN.md](docs/TEST_PLAN.md).

For a read-only desktop check of a file, build with CMake and run `build/midi_file_check /path/to/song.mid`. It validates the complete file and simulates two buffered plays through the real scheduler/registry with four output channels. It reports channel attacks/releases and fails on dropped events or remaining note owners. This does not measure physical SD latency or DIN bandwidth. Diagnostic USB console summaries also include filename, player state, validation percentage and playback time.

## General MIDI Learn

Fn+G or Options → Learn Control captures a note, CC, program change, channel pressure, or pitch bend. Choose action, trigger, relative encoding, and whether it consumes MIDI; select Save Mapping and press Enter. Options → Edit Mapping edits the selected mapping; Mapping/Delete removes it. Left/right select the mapping index. New/edited mappings queue a profile save; deletions require Save Profile. Source number/channel/cable are captured exactly. To change a selector, delete it and learn again. Advanced ranges/match metadata can be edited in SD JSON, then reloaded from Options. File browse/load/play/stop also use semantic actions, so MIDI mappings and Cardputer controls share the same execution path.

## MIDI loops and presets

Choose free length or 1/2/4/8/16 bars, then press Z before playing. Fixed-length recording starts immediately and begins playback at its boundary. In free mode, press Z or Space to close the recording. Space stops playback; Fn+Z enables/disables overdub; Fn+U stops and removes the latest overdub layer; Fn+Backspace clears. Recording again replaces the in-memory loop. There is no count-in or audio recording.

Loops store post-engine channel MIDI, including enabled performance/bass/raw streams, with original output channels. Playback does not get transformed a second time. Recording uses 96 ticks per quarter and optional 1/4, 1/8, 1/8T, 1/16, 1/16T, or 1/32 onset quantization. Note durations are retained and clipped at the cycle boundary. Capacity is 512 note/control entries and 128 pending notes; overflow is counted. Free length is bounded to 1024 bars. Loops retain their recorded BPM; changing the live BPM does not time-stretch an existing loop. Stop or re-record when changing tempo.

The screen shows loop state and entry count. Loop playback has separate note ownership; stopping it does not release a live-held copy of the same pitch. Loop sustain is reset on loop-used sustain channels at stop/wrap; put live sustained playing on a separate channel if independent pedal behavior is needed. Panic stops all playback.

Options offers Save Loop/Load Loop and 16 shared numbered preset/loop slots. Fn+S/Fn+L save/load the selected musical preset slot, not controller-profile mappings or loops; use Save Profile/Reload Profile for mappings. Files are stored as `/M5Chord/presets/preset-01.json` and `/M5Chord/loops/loop-01.json` through slot 16. Save targets are captured when requested; storage waits for released notes, stopped playback, and an empty DIN queue. Only one session-storage request can be pending. Loads stop active notes first. Invalid schemas, checksums, ranges, or unavailable SD fail visibly. Temporary files and `.bak` files protect replacement; real power-loss recovery still needs physical testing.

Settings auto-save to internal NVS without SD. A first installation with no valid saved settings starts in **CHORD mode** and **Keyboard view**. Valid existing settings keep the user's selected mode, including KEY or BYPASS. View changes now persist separately in internal storage and do not change preset schemas. Older builds did not save a view preference, so an upgrade with no saved view starts in Keyboard view; choose another view once to retain it afterward. Allow at least 1.5 seconds after changing a preference and stop playback/release notes before powering off so the idle save can finish. Loops require explicit SD save and load and never auto-play after boot. Missing SD disables only SD-dependent operations.

Version 1.1.0 migrates schema-2 settings/presets with AUTO input and four layer channels. It saves schema 3 under a separate internal `settings-v3` key, retaining the older `settings-v2` record for downgrades. An older 1.0 build will not load new schema-3 SD presets and will see its earlier internal settings rather than subsequent changes.

## Diagnostics

Options → Export Diagnostics writes `/M5Chord/logs/diagnostics.json`: transport identity, counts, scheduler lateness, DIN high water, loop drops, and a 64-sample/one-second runtime history. Collection uses a fixed RAM ring; file output is deferred until idle. Adjacent expression messages are coalesced without crossing note events; Note Ons can displace coalescible traffic. If an exhausted DIN queue cannot accept a critical release, the next main-loop pass clears queued traffic and issues a full panic. BLE queue overflow discards the incomplete input batch and invokes panic. These paths are safety recovery, not a claim of lossless MIDI under unlimited load.

## Architecture

The engine under `src/app`, `src/engine`, `src/midi`, `src/controller`, and `src/scheduler` is ordinary allocation-free C++ in the performance path. Hardware dependencies stay under `src/transport`, `src/hardware`, `src/storage`, and `src/ui`.

The native ESP-IDF USB Host Library scans the active configuration, bounds-checks descriptors, and selects a claimable MIDIStreaming interface/alternate setting with an input endpoint. It ignores Audio Streaming interfaces, accepts bulk or interrupt endpoints, and does not require a hardcoded VID/PID. It does not switch USB configurations or support external hubs. MIDI arrives as normalized `MidiEvent` values. Every scheduled note carries a `VoiceId` and stream owner. Physical channel/pitch reference counting protects overlapping live and loop voices.

## Known limitations

- A startup issue remains with a tested BLE controller; see [device-specific test notes](#device-specific-test-notes) for its identity and workaround. The 1.1.0 handshake changes are not a verified fix.
- Cardputer 1.0 compatibility was reported working; 1.1 physical testing and exhaustive per-release hardware/latency checks remain outstanding.
- One USB device and the first claimable MIDIStreaming interface are supported at a time.
- BLE MIDI supports one automatically discovered standard BLE-MIDI peripheral at a time and has no on-device chooser.
- USB is host input, not a USB-MIDI device output to a DAW. BLE is controller input only.
- No SysEx forwarding, external-clock-follow mode, ORC secret-chord tables, or factory-pattern library.
- Loops retain recorded tempo/channels, and notes crossing a loop boundary are clipped. There is no count-in or file naming UI.
- SD MIDI-file playback has explicit file-size, format, track and simultaneous-note limits and remaps selected source channels; see its section above. It does not preserve a General MIDI drum-channel reservation.
- No synth, audio, effects, drum sounds, or waveform display.
- The scheduler is deterministic and bounded, but sustained DIN saturation needs physical stress measurement.

See [docs/HARDWARE.md](docs/HARDWARE.md), [docs/MIDI_BEHAVIOR.md](docs/MIDI_BEHAVIOR.md), [docs/USB_DEBUG.md](docs/USB_DEBUG.md), and [docs/TEST_PLAN.md](docs/TEST_PLAN.md) before hardware testing.

## Troubleshooting

| Symptom | Check |
|---|---|
| BT connected, no notes | Enable the controller's BLE-MIDI mode, disconnect other BLE hosts, and select BLE input or reset AUTO selection. Check `N`, `E` and `RX`. See the device-specific test notes for the tested startup issue and workaround. |
| No USB detection | Use a data cable, direct OTG connection and the controller's USB-MIDI mode. External hubs, including powered hubs, are unsupported. Try a supported BLE or DIN input instead. |
| No DIN input | Connect source MIDI OUT to Unit INPUT, select DIN input and inspect `DIN E`. Verify Grove wiring and the Unit hardware-revision caveat above. |
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

For a bug report, include Cardputer revision, M5Chord version, controller model/firmware, selected input (USB/BLE/DIN/AUTO), power/cable/hub setup, SD presence, receiver and MIDI channel, exact steps, and Geek counters. Attach diagnostics if available; review them before posting because they contain device/profile identity strings.

## License and credits

M5Chord source is MIT-licensed; see [LICENSE](LICENSE). Dependencies and bundled font data retain their own licenses; see [third-party notices](THIRD_PARTY_NOTICES.md). Thanks to M5Stack, Espressif, Arduino, the library authors and font designers. Controller inspiration and remaining differences are recorded in the [ORC-1 assessment](docs/ORC1_CONTROLLER_ASSESSMENT.md); M5Chord does not include synth features.
