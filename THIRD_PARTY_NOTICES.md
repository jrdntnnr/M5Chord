# Third-party notices

M5Chord's own source is MIT-licensed. Dependencies, derived drivers and fonts keep their upstream terms; the project license does not relicense them. Release archives include the notices in `licenses/` and the pinned `platformio.ini`.

| Component | Pinned version | License / source |
|---|---|---|
| M5Cardputer | 1.1.1 | MIT SPDX in source; [upstream](https://github.com/m5stack/M5Cardputer/tree/1.1.1) |
| M5Unified | 0.2.21 | MIT; [upstream](https://github.com/m5stack/M5Unified/tree/0.2.21) |
| M5GFX | 0.2.28 | MIT with third-party components/font notices; [upstream](https://github.com/m5stack/M5GFX/tree/0.2.28) |
| IRremote | 4.7.1 | MIT; [upstream](https://github.com/Arduino-IRremote/Arduino-IRremote/tree/v4.7.1) |
| ArduinoJson | 7.4.3 | MIT; [upstream](https://github.com/bblanchon/ArduinoJson/tree/v7.4.3) |
| Arduino-ESP32 | 2.0.16 | LGPL-2.1 and component-specific terms; [source](https://github.com/espressif/arduino-esp32/tree/2.0.16) |
| ESP32 BLE Arduino | Bundled with Arduino-ESP32 | Apache-2.0 notice included |
| ESP-IDF | Arduino-ESP32 2.0.16 SDK | Apache-2.0 and component-specific terms; [upstream license policy](https://github.com/espressif/esp-idf/blob/v4.4.7/LICENSE) |
| Adafruit TCA8418 derived driver | Bundled with M5Cardputer | BSD-3-Clause; [original source](https://github.com/adafruit/Adafruit_TCA8418) |
| Orbitron font data | Bundled with M5GFX | SIL Open Font License 1.1; [font project](https://github.com/theleagueof/orbitron) |

M5Cardputer's package identifies its own code with MIT SPDX headers but does not ship a standalone license file; `licenses/M5Cardputer-MIT.txt` reproduces the MIT terms with that copyright attribution. The Adafruit and Orbitron notices are retained separately. Bundled GFX font notices are included even where a font is not selected by M5Chord.

The complete M5Chord source for release 1.0 is tag `v1.0` in this repository. The source build instructions fetch the pinned dependencies, permitting a full rebuild with modified libraries. No device flash dump, credentials, private controller profiles or settings are included. See each upstream source for component-level notices and corresponding source; do not remove those notices when redistributing modified firmware.
