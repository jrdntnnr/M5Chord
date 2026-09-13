# M5Chord 1.0

Initial public release of M5Chord: a standalone harmonic MIDI controller for Cardputer 1.0, 1.1 and ADV. One universal image automatically selects the classic or ADV keyboard driver. MIDI controller only; no synth or audio features.

## Included

- BYPASS, CHORD and automatic scale-based KEY modes; four playstyles.
- DIM/MIN/MAJ/SUS qualities and independent 6/m7/M7/9 additions, momentary or stacked.
- Block, strum, two-octave strum, slop, arp, two-octave arp, pattern and harp.
- Eight Western scale options and four explicitly 12-tone Arabic-inspired scale approximations.
- Bluetooth MIDI and direct USB-MIDI input, DIN output through Unit MIDI in SEPARATE mode.
- MIDI Learn, nine-pad wizard, output channel/layer selection, profiles, 16 preset/loop slots and a MIDI looper.
- Retro green/amber display, large chord names, Tab Options and live output-keyboard feedback.
- Shared revision-safe pin definitions and key-swap/release handling.
- M5Chord name and version on startup, BLE local identity and diagnostic export. Existing settings and SD paths are retained.

## Known SMK-37 Bluetooth startup bug

The SMK-37 can show connected while no keys register. **Switch the SMK-37 off and back on twice while M5Chord stays running**, then play a key and verify receive activity. This is the user-reported workaround. Disconnect other BLE hosts before trying it.

**A fix is planned for a coming version. It is not fixed in 1.0.** A green BT indicator alone does not prove MIDI input is arriving.

## Install

Download `M5Chord-v1.0.zip` for all binaries, checksums, full instructions and licenses. `M5Chord-v1.0-universal.bin` is the merged fresh-install image for address `0x0000` and resets internal NVS settings. It does not erase microSD contents. Back up settings first.

For an existing build of this project, use the four separate components and the settings-preserving command in the [README](../README.md#install-the-10-binary). The app file belongs at `0x10000`, never zero. No whole-chip erase is needed. Release G0 and power-cycle after uploading.

## Validation and limits

Local CMake/CTest, ASan/UBSan, both pinned PlatformIO environments and package tests passed. Merged segments, offsets and checksums are verified by the packaging script.

The preceding universal compatibility build was flashed to a user-identified Cardputer 1.0 and reported working. Earlier ADV builds also have user observations. The renamed release binary is newly built, not a new physical acceptance claim; Cardputer 1.1 and exhaustive per-revision tests remain pending. See [hardware results](SMK37_TEST_RESULTS.md).

External USB hubs are unsupported. Direct SMK-37 USB interoperability is not established; use BLE with the startup workaround. No SysEx forwarding, external MIDI clock following, quarter-tone tuning, audio synthesis or USB-MIDI output to a DAW. M5Burner publication is separate and has not been performed.
