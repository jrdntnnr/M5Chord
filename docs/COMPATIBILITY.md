# M5Chord universal Cardputer compatibility

The `cardputer-universal` target produces one ESP32-S3 application for Cardputer 1.0, 1.1 and ADV. The old `cardputer-adv` target remains an alias using the same configuration. No user-selected hardware mode or per-revision musical code is needed.

## Hardware boundary

| Revision | Module / flash | Keyboard selected by M5Cardputer | Geek label | This candidate's physical status |
|---|---|---|---|---|
| 1.0 | StampS3 / 8 MB | GPIO matrix | `1.0/1.1` | Compatibility build uploaded and reported working; full release checks pending |
| 1.1 | StampS3A / 8 MB | GPIO matrix | `1.0/1.1` | Not physically tested |
| ADV | StampS3A / 8 MB | TCA8418 over I²C | `ADV` | Not physically tested on this candidate |

The original and 1.1 share the relevant display, keyboard, Grove UART and SD wiring. ADV changes the keyboard interface; M5GFX/M5Unified identify the board family and M5Cardputer 1.1.1 selects its matching reader. Detection does not distinguish 1.0 from 1.1. References: official [Cardputer](https://docs.m5stack.com/en/core/Cardputer), [V1.1](https://docs.m5stack.com/en/core/Cardputer%20V1.1), [ADV](https://docs.m5stack.com/en/core/Cardputer-Adv), and [M5Cardputer API](https://docs.m5stack.com/en/arduino/m5cardputer/program).

All three use the existing 240×135 display, GPIO2 TX / GPIO1 RX for Unit MIDI, and SD CS12 / MOSI14 / MISO39 / SCK40. No PSRAM is required. Speaker, microphone and IMU initialization remain disabled. ADV-only expansion functions and audio are outside this controller's scope.

## Changes

- Removed the SD startup write forcing GPIO5 high: that pin belongs to the classic keyboard matrix.
- Centralized common MIDI/SD pins in `src/hardware/CardputerHardware.h` and added an unsupported-board guard before MIDI/storage startup.
- Process keyboard snapshots every poll instead of gating on the library's key-count-only `isChange()`. Same-count exchanges reach the existing edge dispatcher, including releases; unchanged snapshots do not repeat.
- Added the family label to Geek view and native regressions for pin conflicts, unchanged snapshots, quality/extension swaps and Fn/menu release ownership.
- Kept musical policy, settings schema, controls, fonts and performance UI unchanged.

## Build and verify

```sh
PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-universal
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-adv
python3 -m unittest discover -s tools -p 'test_package_firmware.py'
```

Both environments share pinned configuration. Paths and build metadata can cause differences between independently compiled binaries; distribute the universal artifact unchanged for all three revisions.

Normal upload writes bootloader, partition table, OTA selection and application, not NVS settings or SD contents. Select the actual connected download-mode port:

```sh
PLATFORMIO_CORE_DIR="$PWD/.pio" pio run -e cardputer-universal -t upload --upload-port /dev/cu.usbmodem12101
```

Release G0 and power-cycle afterward. Native USB becomes a host; disappearance of the programming port is not proof of successful application startup. Screen/key/DIN checks remain necessary.

## Distribution

The [M5Chord 1.1.0 release package](https://github.com/jrdntnnr/M5Chord/releases/tag/v1.1.0) contains:

| File | Address | Use |
|---|---|---|
| `M5Chord-v1.1.0-universal.bin` | `0x0000` | Fresh installation; resets NVS settings and OTA selection |
| `M5Chord-v1.1.0-app.bin` | `0x10000` | Update with this project's matching partition layout and app0 selected |
| `bootloader.bin`, `partitions.bin`, `boot_app0.bin` | `0x0000`, `0x8000`, `0xE000` | Separate-component update with the app, retaining NVS |
| `SHA256SUMS` / `manifest.json` | — | Integrity, offsets, input hashes and acceptance status |

The factory image comes from build products, not a user's flash dump. Its gaps overwrite NVS (`0x9000–0xDFFF`) with erased bytes, destroying saved device settings. Back up before installation. It does not erase the SD card or the entire chip. App-only updates do not select the active OTA slot; prefer normal source upload if uncertain.

To package another build, use PlatformIO's Python interpreter with esptool dependencies installed and choose a new output directory:

```sh
python3 tools/package_firmware.py --core-dir .pio --output dist/M5Chord-v1.1.0 --version 1.1.0
```

On this Mac that interpreter is `/usr/local/opt/python@3.10/bin/python3.10`. Packaging verifies the pinned 8 MB partition table/checksum, segment bounds, exact merged segment bytes and empty factory NVS. Ten Python regression tests cover valid and malformed inputs. Merge format: [Espressif esptool](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-commands.html).

The factory binary is a candidate for M5Burner's USER CUSTOM → Publish workflow. Account publication, device-family catalog placement and M5Burner installation still need validation. `manifest.json` is project metadata, not an M5Burner import schema. No publication was performed. See [M5Burner publishing](https://docs.m5stack.com/en/uiflow/m5burner/publish).

## Remaining acceptance

Run the per-revision checklist in [TEST_PLAN.md](TEST_PLAN.md) and record observations in [SMK37_TEST_RESULTS.md](SMK37_TEST_RESULTS.md). The user reported the pre-rename compatibility build working on Cardputer 1.0. This is not three-device functional acceptance; earlier ADV results belong to their recorded hashes.

External USB hubs remain unsupported by the pinned ESP-IDF 4.4 host. BLE remains the SMK-37 path, with a known startup bug: the keyboard may require two off/on cycles before key input works. A fix is planned for a coming version. Revision compatibility does not fix SMK USB interoperability, guarantee USB VBUS power or add audio output.
