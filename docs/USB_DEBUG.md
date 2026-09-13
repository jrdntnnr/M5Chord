# USB debugging

Geek view displays the current VID, PID, claimed interface number, input endpoint, received packet count, scheduler depth, lateness, drops, and active-note count.

The host scans the active configuration descriptor. It recognizes Audio class subclass 3 as MIDIStreaming, claims only that interface, and ignores AudioControl and AudioStreaming. It accepts an inbound bulk or interrupt endpoint and submits a reusable transfer buffer. No controller VID/PID is hardcoded.

## Symptoms

`USB` remains grey: verify the cable carries data, the controller is charged, and the Cardputer is no longer connected to the programming computer.

Connect/disconnect cycling: inspect the power path before changing firmware. Do not insert a USB hub with the current ESP-IDF 4.4 build because external hubs are unsupported. Use a single-device USB 2.0 power injector if external VBUS is required.

The current build supports one device directly attached to the root port. A green USB indicator while a hub is attached identifies the hub, not a MIDI controller behind it.

VID/PID visible but no interface: the active configuration does not expose a claimable class-compliant MIDIStreaming interface. Record the complete observed topology in `SMK37_TEST_RESULTS.md`.

Interface visible but no notes: inspect endpoint direction/type and test whether the controller uses another alternate setting or interface.

Stuck notes after removal: record the active-note and panic counters immediately before and after removal. This is a release-blocking defect.

USB serial is intentionally disabled because the native connector is the host port. The on-device Geek screen is the primary initial diagnostic channel.
