# Hardware setup

Set the M5 Unit MIDI front switch to `SEPARATE`. The firmware uses the Unit only as a UART-to-DIN transport and produces no audio.

| Cardputer 1.0 / 1.1 / ADV | M5 Unit MIDI |
|---|---|
| GND | GND |
| 5 V | 5 V |
| GPIO2 / TX | UART_RX |
| GPIO1 / RX | UART_TX |

UART configuration is 31,250 baud, 8 data bits, no parity, one stop bit. GPIO2 carries MIDI output. In 1.1.0 development firmware, GPIO1 also receives MIDI from the Unit's DIN IN optocoupler. Select Tab → MIDI input → DIN or AUTO. Keep SEPARATE selected for processed DIN output; BYPASS is a physical thru path. This uses the Unit MIDI/SAM2695 board as a MIDI interface, not as an audio synth. The [schematic](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/774/SCH_UnitMIDI_B04_sch_2024_07_08_15_41_29.pdf) shows input wired to Grove RX independently of the switch; physical input acceptance on the actual Unit revision remains pending.

The native Cardputer USB-C port operates in host mode after boot. Fully charge battery-powered controllers before connecting them. The pinned Arduino-ESP32 2.0.16 and ESP-IDF 4.4 host stack does not support devices behind an external USB hub. Use a direct OTG data path. If external power is required, use a single-device USB 2.0 power injector that preserves the direct data connection and prevents supply backfeed.

BLE MIDI is an alternative input path and leaves the USB-C connector available for power. The Cardputer scans for any advertised standard BLE-MIDI service, with MIDI-name and legacy SMK BLE identity fallbacks; GATT validates the service and characteristic. Ensure the controller is not connected to another Bluetooth MIDI host. Geek view and bounded diagnostic logs show setup, read handshake, notifications and errors. The diagnostic build uses USB serial instead of USB host; restore the universal build to test USB controllers. Low-powered, simple class-compliant USB controllers connected directly are the intended USB input case, not a guarantee for every model.

The microSD bus uses CS GPIO12, MOSI GPIO14, clock GPIO40, and MISO GPIO39 on all three revisions. GPIO5 belongs to the original/1.1 keyboard matrix and must not be driven by SD initialization. The universal build removes the earlier GPIO5 write. M5Cardputer selects the original matrix or ADV TCA8418 keyboard reader at runtime. See [compatibility](COMPATIBILITY.md) for the source references and physical test matrix.

## Safe bring-up

1. Boot without USB or DIN devices and confirm the display.
2. Connect Unit MIDI and a MIDI monitor or known receiver.
3. Verify UART/DIN using the checklist in `TEST_PLAN.md`.
4. Connect a charged controller directly through a known data cable, or enable its BLE MIDI mode.
5. Verify USB VID, PID, interface, and endpoint, or BLE connection and event counters in Geek view.
6. Test bypass before enabling chord or performance modes.
