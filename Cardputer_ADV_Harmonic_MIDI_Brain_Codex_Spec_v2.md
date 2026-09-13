# Cardputer ADV Harmonic MIDI Brain
## Codex Build Specification

**Status:** Build specification / implementation brief  
**Primary hardware:** M5Stack Cardputer ADV (K132-Adv)  
**Initial USB controller target:** M-VAVE SMK-37 family (initial physical test device)  
**MIDI output hardware:** M5Stack Unit MIDI, used only as a UART-to-DIN MIDI transport  
**Audio generation:** Explicitly out of scope  
**Project type:** Standalone USB-MIDI host → MIDI transformation engine → DIN MIDI output appliance  
**Design inspiration:** Telepathic Instruments Orchid ORC-1 interaction model, independently implemented

**Revision:** 2  
**Revision 2 additions:** Orchid-style performance UI, SMK-37 pad mapping for the eight Orchid chord controls, and SD-card custom MIDI controller profiles with MIDI Learn.

---

# 0. Agent directive

Build this as a real, flashable Cardputer ADV firmware project, not as a design mockup.

The device must act as a standalone MIDI processing brain between two external devices:

```text
M-VAVE SMK-37
(class-compliant USB MIDI)
        │
        │ USB-C / USB host
        ▼
┌──────────────────────────────┐
│ M5Stack Cardputer ADV        │
│                              │
│ USB MIDI host                │
│      ↓                       │
│ MIDI parser                  │
│      ↓                       │
│ Harmonic/chord engine        │
│      ↓                       │
│ Voicing engine               │
│      ↓                       │
│ Performance engine           │
│      ↓                       │
│ Optional bass/raw streams    │
│      ↓                       │
│ Scheduler / MIDI router      │
└───────────────┬──────────────┘
                │
                │ UART MIDI @ 31,250 baud
                ▼
┌──────────────────────────────┐
│ M5Stack Unit MIDI            │
│ switch = SEPARATE            │
│                              │
│ DIN MIDI OUT ────────────────┼──► External MIDI device
└──────────────────────────────┘
```

The M5 Unit MIDI's internal SAM2695 is **not a sound source for this project**. Do not initialize, configure, expose, or build UI around the SAM2695. Do not build an audio path. The Unit is being used because it provides the physical DIN MIDI interface.

The finished firmware must be useful even if no audio hardware exists in the Cardputer or Unit.

Prioritize, in this order:

1. MIDI correctness and no stuck notes.
2. Reliable USB host enumeration of the SMK-37.
3. Low and deterministic latency.
4. Immediate physical controls.
5. Clear, minimal UI.
6. Advanced generative features.
7. Persistence/looping.

Do not block the main loop with `delay()` for musical timing.

---

# 1. Product definition

The Cardputer is a **MIDI transformation appliance**.

It accepts live MIDI from a USB controller and transforms simple note input into harmonically structured MIDI output.

The defining workflow is:

> Play a root note or melody on a normal keyboard; use the Cardputer controls to define harmony, voicing and performance behavior; send the resulting MIDI to any external DIN-MIDI instrument.

It should feel more like a playable harmonic instrument than a menu-driven sequencer.

The first controller used for testing is the M-VAVE SMK-37. The implementation must remain USB-MIDI class-compliant and must not hardcode the product's VID/PID.

---

# 2. Non-goals

Do **not** implement the following unless later requested:

- Internal synthesis.
- SAM2695 patch editing.
- Cardputer speaker/headphone sound.
- Audio recording.
- Audio looping.
- USB audio host support.
- DAW functionality.
- Sample playback.
- Wi-Fi MIDI.
- RTP-MIDI.
- BLE MIDI in the MVP.
- MIDI 2.0 features beyond graceful detection/fallback.
- Exact reverse engineering of proprietary Orchid firmware.
- Proprietary Orchid "Secret Chord" tables or proprietary factory pattern data.

All harmonic/performance behavior should be independently implemented from conventional music theory and the functional behavior described in public Orchid documentation.

---

# 3. Hardware assumptions

## 3.1 Cardputer ADV

Target:

- M5Stack Cardputer ADV / K132-Adv
- ESP32-S3FN8
- 240 MHz dual core
- 8 MB flash
- 240×135 ST7789V2 display
- 56-key Cardputer keyboard
- TCA8418 keyboard controller
- microSD
- BMI270 IMU
- 1750 mAh battery
- Grove/HY2.0-4P port:
  - GND
  - 5 V
  - GPIO2
  - GPIO1

The Cardputer ADV has already been demonstrated by third-party firmware operating the ESP32-S3 native USB peripheral in USB-host mode. The implementation must use the native USB OTG peripheral, not software USB.

## 3.2 M5Stack Unit MIDI

Use the Unit MIDI only as a DIN MIDI transport.

Required operating mode:

```text
UNIT MIDI FRONT SWITCH: SEPARATE
```

Grove wiring:

```text
Cardputer ADV        M5 Unit MIDI
-------------        -------------
GND            ───►  GND
5 V            ───►  5 V
GPIO2 / TX     ───►  UART_RX
GPIO1 / RX     ◄───  UART_TX
```

Initialize the UART as:

```text
baud:     31250
format:   8N1
RX pin:   GPIO1
TX pin:   GPIO2
```

Conceptual Arduino call:

```cpp
Serial2.begin(31250, SERIAL_8N1, 1, 2);
```

The firmware should send raw MIDI bytes directly over this UART.

Do **not** require the M5-SAM2695 library.

Important hardware behavior: in SEPARATE mode the Unit routes Cardputer TX to the DIN MIDI OUT and also internally toward the SAM2695. That internal connection is irrelevant; the audio output is unused.

## 3.3 SMK-37

Initial target device: M-VAVE SMK-37 family.

Known useful characteristics of current SMK-37 models:

- 37 velocity-sensitive keys.
- USB-C MIDI.
- Class-compliant MIDI operation.
- Pitch control.
- Modulation control.
- Sustain support.
- Pads/encoders/faders depending on model.
- Current models may enumerate as a composite USB device containing both MIDI and USB audio interfaces.

Therefore:

**Do not assume that the entire USB device is a MIDI-only device.**

The USB host layer must:

1. enumerate the USB device;
2. inspect interfaces/descriptors;
3. locate a MIDI Streaming interface/endpoints;
4. claim only the MIDI interface;
5. ignore USB Audio interfaces;
6. log manufacturer/product/VID/PID/interface information for diagnostics.

Do not hardcode an SMK-37 VID/PID. Accept any standards-compliant USB MIDI controller by default.

## 3.4 USB power caveat

The SMK-37 contains a rechargeable battery and may attempt to charge when attached over USB.

For initial testing:

- Fully charge the SMK-37 first.
- Prefer a known-good data cable.
- If the Cardputer browns out, resets, fails enumeration, or becomes unstable, use a powered USB OTG hub.
- Do not "solve" host instability in software if the actual problem is excessive VBUS load.
- Provide a diagnostics screen/log entry for USB connect/disconnect loops.

A direct USB-C-to-USB-C cable may be less reliable for host-role negotiation on some ESP32-S3 board implementations. Support/testing should include a USB OTG adapter/hub path.

---

# 4. Recommended software architecture

Use a layered architecture so the musical engine has no dependencies on M5Stack, USB, display or FreeRTOS.

```text
┌──────────────────────────────────────┐
│ Platform / HAL                       │
│ USB host | UART | keys | LCD | NVS   │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│ MIDI Transport Layer                 │
│ UsbMidiSource      DinMidiSink       │
└──────────────────┬───────────────────┘
                   │ MidiEvent
                   ▼
┌──────────────────────────────────────┐
│ MIDI Router                          │
│ filter / bypass / channel mapping    │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│ Harmonic Engine                      │
│ chord | key | extensions | voicing   │
└──────────────────┬───────────────────┘
                   │ GeneratedVoice
                   ▼
┌──────────────────────────────────────┐
│ Performance Engine                   │
│ block/strum/slop/arp/pattern/harp    │
└──────────────────┬───────────────────┘
                   │ ScheduledMidiEvent
                   ▼
┌──────────────────────────────────────┐
│ Scheduler                            │
│ monotonic µs timestamps, no delays   │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│ DIN MIDI Sink                        │
│ raw UART bytes @ 31250               │
└──────────────────────────────────────┘
```

The engine must compile and run as ordinary C++ in host-side unit tests.

---

# 5. Repository structure

Use this approximate structure:

```text
cardputer-midi-brain/
├── README.md
├── AGENTS.md
├── LICENSE
├── platformio.ini                  # if PlatformIO path chosen
├── CMakeLists.txt                  # if ESP-IDF path chosen
├── sdkconfig.defaults              # if ESP-IDF
│
├── src/
│   ├── main.cpp
│   │
│   ├── app/
│   │   ├── App.h
│   │   ├── App.cpp
│   │   ├── AppState.h
│   │   └── AppState.cpp
│   │
│   ├── midi/
│   │   ├── MidiEvent.h
│   │   ├── MidiParser.h
│   │   ├── MidiParser.cpp
│   │   ├── MidiRouter.h
│   │   ├── MidiRouter.cpp
│   │   ├── MidiConstants.h
│   │   └── ActiveNoteRegistry.h
│   │
│   ├── engine/
│   │   ├── ChordEngine.h
│   │   ├── ChordEngine.cpp
│   │   ├── ScaleEngine.h
│   │   ├── ScaleEngine.cpp
│   │   ├── VoicingEngine.h
│   │   ├── VoicingEngine.cpp
│   │   ├── PerformanceEngine.h
│   │   ├── PerformanceEngine.cpp
│   │   ├── BassEngine.h
│   │   ├── BassEngine.cpp
│   │   ├── ClockEngine.h
│   │   ├── ClockEngine.cpp
│   │   ├── LoopEngine.h
│   │   └── LoopEngine.cpp
│   │
│   ├── scheduler/
│   │   ├── MidiScheduler.h
│   │   └── MidiScheduler.cpp
│   │
│   ├── transport/
│   │   ├── MidiSource.h
│   │   ├── MidiSink.h
│   │   ├── UsbMidiSource.h
│   │   ├── UsbMidiSource.cpp
│   │   ├── DinMidiSink.h
│   │   └── DinMidiSink.cpp
│   │
│   ├── hardware/
│   │   ├── CardputerHal.h
│   │   ├── CardputerHal.cpp
│   │   ├── InputKeys.h
│   │   └── InputKeys.cpp
│   │
│   ├── ui/
│   │   ├── Ui.h
│   │   ├── Ui.cpp
│   │   ├── Screens.h
│   │   └── Screens.cpp
│   │
│   ├── storage/
│   │   ├── SettingsStore.h
│   │   ├── SettingsStore.cpp
│   │   ├── PresetStore.h
│   │   └── PresetStore.cpp
│   │
│   └── diagnostics/
│       ├── Diagnostics.h
│       └── Diagnostics.cpp
│
├── test/
│   ├── test_chords.cpp
│   ├── test_scales.cpp
│   ├── test_voicing.cpp
│   ├── test_note_lifecycle.cpp
│   ├── test_performance.cpp
│   ├── test_scheduler.cpp
│   └── test_router.cpp
│
└── docs/
    ├── HARDWARE.md
    ├── MIDI_BEHAVIOR.md
    ├── USB_DEBUG.md
    └── TEST_PLAN.md
```

Exact framework choice may change the HAL folders, but preserve the separation.

---

# 6. Framework choice

## 6.1 Preferred prototype route

For the fastest first hardware proof:

- Arduino-compatible framework on ESP32-S3.
- M5Cardputer/M5Unified for Cardputer display/keyboard.
- A USB MIDI host implementation that supports class-compliant MIDI.
- PlatformIO if dependency versions can be made deterministic.

Candidate USB host libraries may be evaluated, but they must not leak their API into the music engine.

Create this interface:

```cpp
class MidiSource {
public:
    virtual ~MidiSource() = default;
    virtual bool begin() = 0;
    virtual void poll() = 0;
    virtual bool connected() const = 0;
};
```

and deliver normalized `MidiEvent` objects to the application.

## 6.2 USB-host fallback path

If the selected Arduino USB MIDI host library fails to enumerate the actual SMK-37 because it is a composite Audio+MIDI USB device:

- do not abandon the architecture;
- replace only `UsbMidiSource`;
- use the ESP-IDF USB Host Library directly;
- parse configuration descriptors;
- locate Audio class / MIDIStreaming subclass interfaces;
- claim the MIDI interface;
- submit interrupt/bulk transfers as appropriate;
- decode USB-MIDI event packets.

A native ESP-IDF Cardputer ADV USB-host project is known to be feasible, so this is an engineering fallback, not a hardware redesign.

## 6.3 Dependency policy

After the first successful hardware build:

- pin the exact toolchain;
- pin Arduino/ESP-IDF version;
- pin M5 libraries;
- pin USB host library commit/version;
- check dependency versions into README.

Do not leave the project dependent on "latest".

---

# 7. Core MIDI event representation

Normalize USB MIDI into a transport-neutral representation.

```cpp
enum class MidiType : uint8_t {
    NoteOff,
    NoteOn,
    PolyAftertouch,
    ControlChange,
    ProgramChange,
    ChannelPressure,
    PitchBend,
    Clock,
    Start,
    Continue,
    Stop,
    SongPosition,
    SysEx,
    Unknown
};

struct MidiEvent {
    MidiType type;
    uint8_t channel;      // 0..15 where applicable
    uint8_t data1;
    uint8_t data2;

    // Pitch bend normalized to 0..16383.
    uint16_t value14;

    // Monotonic receive timestamp.
    uint64_t timestamp_us;

    // Optional source identity.
    uint8_t cable;
};
```

Normalize Note On with velocity zero into Note Off.

Do not allocate memory per incoming event.

---

# 8. Operating modes

The top-level engine requires three musical modes.

## 8.1 BYPASS

Raw controller behavior for debugging and ordinary play:

```text
USB MIDI IN → DIN MIDI OUT
```

Preserve:

- channel;
- notes;
- velocities;
- CC;
- program change;
- pitch bend;
- channel pressure;
- poly aftertouch where possible;
- sustain;
- real-time MIDI messages.

This is the first hardware milestone.

## 8.2 CHORD

Each source Note On is interpreted as a chord root.

The Cardputer-selected chord formula is applied.

Example:

```text
Input: C3
Mode: Major + M7 + 9

Output:
C3 E3 G3 B3 D4
```

Source Note Off releases the generated voice associated with that exact source note.

Multiple root notes may be active concurrently.

## 8.3 KEY

A selected musical key/scale determines the diatonic harmony generated from each root.

Example:

```text
Key: C major

C → C major
D → D minor
E → E minor
F → F major
G → G major
A → A minor
B → B diminished
```

Manual chord-quality controls may temporarily override the derived quality.

---

# 9. Chord engine

## 9.1 Base chord qualities

MVP chord qualities:

```text
Major      [0, 4, 7]
Minor      [0, 3, 7]
Diminished [0, 3, 6]
Suspended  [0, 5, 7]   // Sus4
```

Default on boot: Major.

Only one base quality is active at a time.

## 9.2 Additive extensions

Support additive extension switches:

```text
6   → +9 semitones
m7  → +10 semitones
M7  → +11 semitones
9   → +14 semitones
```

Extensions can be combined.

Examples:

```text
C major + 6       = C E G A
C major + M7      = C E G B
C major + m7 + 9  = C E G Bb D
C minor + m7 + 9  = C Eb G Bb D
```

Maintain musically sensible register ordering: the ninth should initially appear above the octave, not as D directly above C unless voicing changes it.

Remove exact duplicate MIDI pitches.

Clamp or shift octave placement to remain in MIDI range 0..127.

## 9.3 Future chord library

Architecture must make later additions data-driven:

- sus2
- aug
- add9
- 11
- 13
- maj9
- min9
- 7sus4
- half-diminished
- diminished seventh
- altered dominant
- user chord recipes

Do not hardwire every chord in UI logic.

---

# 10. Scale / key engine

MVP scales:

- Major / Ionian
- Natural minor / Aeolian
- Dorian
- Mixolydian
- Harmonic minor
- Major pentatonic
- Minor pentatonic
- Chromatic

Scale representation:

```cpp
struct ScaleDefinition {
    const char* name;
    std::array<uint8_t, N> semitones;
};
```

For Key mode, construct diatonic harmony using scale degrees rather than naive semitone snapping.

Triad algorithm:

```text
root degree
+ two scale degrees
+ two more scale degrees
```

For seventh chords:

```text
root
+2 scale degrees
+2
+2
```

For ninth:

```text
+2 again
```

This produces genuine scale-derived chord tones.

## Harmonic quantize option

Provide optional `harmonic_quantize`.

When enabled in manual CHORD mode:

- root is never moved;
- non-root chord tones not in the selected scale are moved to a nearest in-scale pitch;
- tie break should minimize total deviation and preserve ascending pitch ordering;
- behavior must be deterministic.

Default: off in CHORD mode, intrinsic in KEY mode.

---

# 11. Voicing engine

This is a major part of the instrument.

Represent voicing as an integer step:

```text
voicing_step = 0 default
range approximately -8 .. +8
```

Base chord is sorted ascending.

Positive voicing step:

```text
move the current lowest note up one octave;
re-sort;
repeat once per step.
```

Negative voicing step:

```text
move the current highest note down one octave;
re-sort;
repeat once per negative step.
```

Example:

```text
0: C3 E3 G3
1: E3 G3 C4
2: G3 C4 E4
3: C4 E4 G4
```

The engine must support any chord size.

When the voicing changes while a source root is held:

1. calculate new generated note set;
2. issue Note Off only for notes no longer present;
3. issue Note On for newly introduced notes;
4. preserve common notes;
5. update active-note registry atomically.

Avoid brute-force panic/retrigger if not required.

## Future option: smooth voice leading

Later enhancement:

Choose the inversion of each newly requested chord that minimizes aggregate voice movement from the previous chord.

Do not make this a requirement for first release.

---

# 12. Performance engine

Performance mode determines how chord notes are emitted.

Required modes:

1. Block
2. Strum
3. Strum 2 Octaves
4. Slop
5. Arp
6. Arp 2 Octaves
7. Pattern
8. Harp

No performance engine may use blocking sleeps.

All generated MIDI goes through the scheduler.

## 12.1 Block

All chord Note On events receive the same logical timestamp.

On root release, release the entire generated voice.

## 12.2 Strum

Sort chord ascending.

Schedule Note On events using:

```text
note[i].time = start + i * strum_interval
```

Default:

```text
strum_interval = 25 ms
```

Configurable range:

```text
2..120 ms
```

Root Note Off must cancel any not-yet-fired scheduled Note Ons belonging to that voice and turn off already-fired notes.

Provide direction:

- Up
- Down

Later:

- Up/Down alternating

## 12.3 Strum 2 Octaves

Generate the selected voicing plus a second octave copy where MIDI range allows.

Deduplicate pitches.

Then apply Strum.

## 12.4 Slop

Humanized strum.

Base behavior = Strum plus deterministic pseudo-random variation:

- timing jitter, default ±8 ms;
- velocity jitter, default ±5;
- seeded PRNG.

Never schedule an event before the initiating event timestamp.

Expose amount 0..100%.

## 12.5 Arp

While one or more roots remain active:

- repeatedly iterate the generated chord notes;
- use master BPM;
- selectable division;
- configurable gate;
- respond immediately to chord/voicing changes at a safe rhythmic boundary.

Required divisions:

```text
1/4
1/8
1/8T
1/16
1/16T
1/32
```

Default:

```text
1/16
gate = 75%
direction = Up
```

Directions:

- Up
- Down
- UpDown
- Random

## 12.6 Arp 2 Octaves

Same as Arp using two-octave note pool.

## 12.7 Pattern

Pattern mode uses explicit note-index sequences instead of simply walking all chord tones.

Examples of **independently designed** pattern data:

```text
[0, 1, 2, 1]
[0, 2, 1, 2]
[0, 1, 3, 2, 1, 2]
[0, 2, 3, 1]
```

Indexes wrap or clamp sensibly for smaller chords.

Represent patterns as data so more can be added without changing scheduler code.

## 12.8 Harp

A fast triggered cascade.

On each root/chord trigger:

1. create 2–3 octaves of chord tones within MIDI range;
2. sort ascending or descending;
3. rapidly emit a single cascade;
4. default spacing 18 ms;
5. note duration should be long enough for tones to overlap.

Unlike Arp, Harp does not continuously repeat by default.

---

# 13. Clock engine

Master clock is required for rhythmic performance modes and looping.

BPM:

```text
minimum: 30
maximum: 300
default: 100
```

Use integer/fixed-point or high-resolution timing to avoid accumulated floating-point drift.

Internal timing base should derive from monotonic microsecond time.

Support standard MIDI clock output:

```text
24 PPQN
```

When MIDI clock output is enabled:

- send `0xF8` at correct 24 PPQN intervals;
- Start sends `0xFA`;
- Stop sends `0xFC`;
- Continue sends `0xFB` if implemented.

MIDI clock generation must not block musical event processing.

Provide tap tempo.

Four taps should establish a stable tempo; reject implausible intervals.

---

# 14. Bass engine

Bass is MIDI only.

Default: off.

Configurable bass output channel, default MIDI Channel 2 (`channel index 1` internally).

Required modes:

### OFF
No generated bass.

### ROOT
On each chord/root trigger, output chord root at configured bass octave.

### LOWEST
Output the lowest note of the current voiced chord, shifted into configured bass range.

### UNISON
Mirror source single notes into the bass channel at configured octave.

Bass octave:

```text
-2 .. +1 relative to source/root
```

When root/chord changes, bass note lifecycle must be independent from performance-channel lifecycle.

---

# 15. MIDI streams and routing

Provide three independently enable-able generated streams:

```text
PERFORMANCE
BASS
RAW_CHORD
```

Defaults:

```text
Performance: ON,  channel 1
Bass:        OFF, channel 2
Raw chord:   OFF, channel 3
```

Channel numbers in UI are 1..16.

Internal representation is 0..15.

## Raw chord stream

Raw chord means the fully constructed and voiced chord without arp/strum/pattern timing behavior.

This makes it possible to drive:

- one patch with the performance pattern;
- another patch with a sustained pad;
- another with bass;

all through the same DIN cable.

## Pass-through controls

Default behavior:

- Sustain CC64: forward.
- Modulation CC1: forward.
- Expression CC11: forward.
- Volume CC7: forward.
- Pitch bend: forward.
- Program change: forward.
- Channel pressure: forward.
- Other CC: forward unless explicitly consumed.

Configurable expression routing:

```text
SOURCE_ONLY
ALL_ACTIVE_GENERATED_CHANNELS
DISABLED
```

Default for pitch/mod/expression:

```text
ALL_ACTIVE_GENERATED_CHANNELS
```

Do not echo Cardputer UI control actions as MIDI CC unless explicitly implemented later.

---


# 16. SMK-37-specific initial behavior

Do not create a proprietary SMK-37 driver. Treat the SMK-37 as a standards-compliant USB MIDI controller and apply a controller profile on top of its raw MIDI stream.

The initial physical test controller is the M-VAVE SMK-37 family. Current models expose assignable velocity-sensitive pads, encoders and faders in addition to the 37-key keyboard. Because those controls are user-assignable, the firmware must **not** hardcode factory note/CC numbers for pads or knobs.

Default behavior for events not consumed by a controller mapping:

| SMK-37 control | Default behavior |
|---|---|
| Piano keys | root/note input |
| Key velocity | propagated into generated notes |
| Pitch control | forwarded |
| Mod control | forwarded |
| Sustain pedal | forwarded |
| Pads | pass through unless consumed by profile |
| Encoders | pass through CC unless consumed by profile |
| Faders | pass through CC unless consumed by profile |
| Transport/buttons | pass through unless consumed by profile |
| Aftertouch | pass through where supported |

Velocity handling for generated chord notes defaults to the source key velocity.

Provide an input note-range filter:

```text
root_input_low
root_input_high
```

Default: `0..127`.

## 16.1 SMK-37 pads = Orchid-style square chord buttons

The first eight SMK-37 pads must control the same eight harmonic functions as Orchid's square chord buttons:

```text
PAD 1 = DIM
PAD 2 = MIN
PAD 3 = MAJ
PAD 4 = SUS

PAD 5 = 6
PAD 6 = m7
PAD 7 = M7
PAD 8 = 9
```

This is the canonical visual arrangement:

```text
┌────────┬────────┬────────┬────────┐
│ DIM    │ MIN    │ MAJ    │ SUS    │
├────────┼────────┼────────┼────────┤
│ 6      │ m7     │ M7     │ 9      │
└────────┴────────┴────────┴────────┘
```

Pads 1–4 select the base chord quality. Pads 5–8 control additive extensions and can be combined.

The pad actions must invoke the **same semantic action path** as Cardputer keys Q/W/E/R and A/S/D/F. There must not be separate SMK-specific chord logic.

### Play Style behavior

Support three controller-interaction styles:

- `SIMPLE`: hold/select a chord type before pressing the root note; changing type requires releasing/retriggering the root.
- `ADVANCED`: a held root can be converted from single-note input into a chord when a chord-type control is pressed.
- `FREE`: like Advanced, but chord types/extensions can be repeatedly changed/retriggered while the root remains held.

Default for this project: `FREE`.

### Extension addition behavior

Provide:

```text
extension_addition = ADD_NOTE | RETRIGGER_CHORD
```

Default: `ADD_NOTE`.

`ADD_NOTE` preserves common chord tones and adds/removes only extension pitches required by the new state. `RETRIGGER_CHORD` safely retriggers the whole generated voice. Both modes must use the active-note registry and never leave stale notes active.

## 16.2 Pads 9–16

Pads 9–16 are not hard-coded in the engine. The bundled SMK profile may optionally map them to performance shortcuts, for example:

```text
PAD 9   = BLOCK
PAD 10  = STRUM
PAD 11  = SLOP
PAD 12  = ARP
PAD 13  = PATTERN
PAD 14  = HARP
PAD 15  = LOOP / REC
PAD 16  = PANIC
```

This second row is configurable and is not part of the required canonical mapping. Only Pads 1–8 are mandated as the eight Orchid-style harmonic buttons.

---

# 16A. Custom MIDI controller profile system

Custom controller support is a core requirement. Any class-compliant USB MIDI device should be usable as a control surface without recompiling firmware.

Mappings live on microSD as human-editable JSON.

Recommended layout:

```text
/midi-brain/
├── controllers/
│   ├── smk37.json
│   ├── generic.json
│   └── my_controller.json
├── presets/
├── loops/
└── logs/
```

If SD is absent, core MIDI bridging and harmonic operation must still work using an embedded fallback profile.

## 16A.1 Profile structure

Example:

```json
{
  "schema": 1,
  "id": "smk37",
  "name": "M-VAVE SMK-37",
  "match": {
    "manufacturer_contains": "M-VAVE",
    "product_contains": "SMK",
    "vid": null,
    "pid": null
  },
  "input": {
    "channel": "omni",
    "root_note_low": 0,
    "root_note_high": 127
  },
  "mappings": [
    {
      "source": {"type":"note","channel":10,"number":36},
      "trigger": "press_release",
      "action": "chord.dim",
      "consume": true
    },
    {
      "source": {"type":"cc","channel":1,"number":21},
      "trigger": "relative",
      "relative_mode": "twos_complement",
      "action": "voicing.delta",
      "consume": true
    }
  ]
}
```

The note/CC numbers above are **examples only**. Do not assume they are SMK-37 defaults.

## 16A.2 Profile matching

Resolve profiles in this order:

1. exact VID+PID, if specified;
2. manufacturer+product string match;
3. product-string match;
4. user-selected profile stored in NVS;
5. generic fallback.

A profile may omit VID/PID so it remains usable across controller firmware revisions.

## 16A.3 Supported source selectors

Support at least:

```text
note
cc
program_change
channel_pressure
pitch_bend
```

Future selectors may include poly-aftertouch, NRPN/RPN and SysEx matching.

Selectors may filter by:

```text
MIDI channel
note / CC / program number
value range
velocity range
press/release edge
USB cable number
```

User-facing channel numbers in JSON are `1..16` or `"omni"`.

## 16A.4 Trigger semantics

Support:

```text
press
release
press_release
toggle
value
relative
```

Definitions:

- `press`: act on Note On or non-zero button-like CC.
- `release`: act on Note Off / release-value CC.
- `press_release`: deliver both down/up state; required for momentary chord controls.
- `toggle`: each activation flips state.
- `value`: pass a normalized continuous value.
- `relative`: decode an endless encoder.

For relative CC encoders support at least:

```text
twos_complement
binary_offset
signed_bit
```

## 16A.5 Semantic action namespace

Profiles may only call known semantic actions. They may not mutate arbitrary application state.

Required action namespace:

```text
# Chord
chord.dim
chord.min
chord.maj
chord.sus
extension.6
extension.m7
extension.M7
extension.9

# Harmonic / key
mode.bypass
mode.chord
mode.key
mode.next
key.toggle
key.root.set
key.next
key.prev
scale.next
scale.prev
harmonic_quantize.toggle
voicing.up
voicing.down
voicing.delta
voicing.set

# Performance
performance.block
performance.strum
performance.strum2
performance.slop
performance.arp
performance.arp2
performance.pattern
performance.harp
performance.next
performance.prev
performance.rate.next
performance.rate.prev
performance.gate.set
performance.amount.set

# Clock / transport
tempo.tap
tempo.set
tempo.up
tempo.down
clock.toggle
transport.start
transport.stop
transport.continue

# Bass / routing
bass.toggle
bass.mode.next
bass.octave.up
bass.octave.down
stream.performance.toggle
stream.bass.toggle
stream.raw_chord.toggle

# Loop
loop.record
loop.play
loop.stop
loop.overdub
loop.undo
loop.clear

# UI / system
view.next
view.prev
preset.next
preset.prev
preset.load
preset.save
panic
```

Unknown actions must be ignored with a diagnostic warning, never crash firmware.

## 16A.6 Event consumption

Every mapping has a `consume` flag.

```json
"consume": true
```

means the MIDI event is a Cardputer command and must **not** also enter the musical/root-note path or pass to DIN output.

This is essential for the SMK chord pads: pressing the pad that selects `MAJ` must not also sound that pad's assigned MIDI note on the downstream instrument.

Processing order:

```text
USB MIDI event
      ↓
controller profile mapper
      │
      ├── semantic action
      │
      └── consumed?
             │
         YES │ NO
             │  └──► ordinary MIDI input/router
             ▼
            stop
```

The profile mapper must run before root-note interpretation.

## 16A.7 SMK eight-pad learning wizard

Because SMK pad assignments are editable, do not invent factory pad MIDI numbers. Provide a guided wizard:

```text
CONTROLLER SETUP
SMK-37

1/8  PRESS PAD FOR: DIM
2/8  PRESS PAD FOR: MIN
3/8  PRESS PAD FOR: MAJ
4/8  PRESS PAD FOR: SUS
5/8  PRESS PAD FOR: 6
6/8  PRESS PAD FOR: m7
7/8  PRESS PAD FOR: M7
8/8  PRESS PAD FOR: 9
```

For each step, capture the first eligible MIDI event and show what was learned, e.g.:

```text
PAD 3
Note 38 / Ch 10
→ MAJ
```

At completion save the real selectors to:

```text
/midi-brain/controllers/smk37.json
```

Use atomic save semantics: write temp → flush/close → rename.

If the same selector is learned twice, warn and ask whether to replace the existing mapping.

## 16A.8 Generic MIDI Learn

After the eight-pad wizard, support arbitrary mapping:

```text
OPTIONS
> Controller
    Active Profile
    Learn Control
    Pad Setup
    Mapping List
    Select Profile
    Reload Profiles
```

Learn flow:

```text
LEARN CONTROL
Press or move a MIDI control…
```

then:

```text
Received:
CC 21 / Ch 1 / Value 127

Map to:
> Voicing Up
```

For continuous controls, allow `value` or `relative` interpretation and normalize to the target action.

The on-device editor only needs to support:

- learn source;
- choose action;
- choose trigger type;
- choose consume yes/no;
- delete mapping;
- save/reload.

Power users may edit JSON directly on SD.

## 16A.9 Parser safety and limits

Treat SD JSON as untrusted configuration.

Suggested limits:

```text
max profile file:     64 KB
max profiles:         32
max mappings/profile: 128
max string field:     64 bytes
```

Reject malformed JSON and invalid ranges. Unknown actions or future fields should be skipped safely. Profile parse failure must fall back to Generic and must never prevent the MIDI engine from starting.

## 16A.10 Reload behavior

`OPTIONS → Controller → Reload Profiles` must:

1. Panic;
2. clear active mapping state;
3. reparse controller files;
4. resolve the connected controller again;
5. return to Performance view.

Do not hot-reload mappings while generated notes remain active.

---

# 17. Active note registry — critical requirement

Stuck-note prevention is a first-class subsystem.

Never infer note ownership only from current settings.

Every generated note must belong to a voice instance.

Suggested identifier:

```cpp
struct VoiceId {
    uint8_t source_channel;
    uint8_t source_note;
    uint32_t generation;
};
```

Track:

```cpp
struct ActiveGeneratedNote {
    VoiceId owner;
    uint8_t output_channel;
    uint8_t note;
    bool note_on_sent;
};
```

When input Note Off arrives:

- find the exact source voice;
- cancel pending scheduled events for that voice;
- send Note Off for every emitted note owned by that voice;
- remove registry entries.

Changes to:

- mode;
- chord quality;
- extensions;
- key;
- voicing;
- performance type;
- output channel;
- routing;
- USB disconnect;

must have explicit note-lifecycle behavior.

## Mandatory panic

Implement:

```text
PANIC
```

Panic sends to all 16 channels:

- CC64 = 0
- CC120 All Sound Off
- CC121 Reset All Controllers
- CC123 All Notes Off

Then explicitly send Note Off for every tracked active pitch.

Then clear:

- active voices;
- scheduler pending note events;
- arp state;
- bass state;
- sustain state;
- loop playback note state if applicable.

Panic must be callable from:

- UI;
- USB disconnect;
- transport fatal error;
- output reconfiguration;
- watchdog recovery path where possible.

---

# 18. Sustain behavior

Initially use downstream sustain semantics.

On CC64:

- forward sustain CC according to expression routing;
- still generate Note Off events normally when source notes are released;
- rely on receiving instrument to sustain them.

On Panic or USB disconnect:

```text
send CC64 = 0 before/allongside All Notes Off
```

Future option may implement internal sustain/latch, but not required for MVP.

---

# 19. Scheduler

Create a fixed-capacity scheduled-event queue.

Requirements:

- monotonic timestamps in microseconds;
- stable ordering for identical timestamps;
- no heap allocation during normal performance;
- cancellation by `VoiceId`;
- cancellation by stream;
- support at least several hundred pending events;
- no `delay()`.

Suggested event:

```cpp
struct ScheduledMidiEvent {
    uint64_t due_us;
    uint32_t sequence;
    VoiceId owner;
    uint8_t stream_id;
    MidiEvent event;
};
```

Data structure options:

- fixed binary min-heap; or
- small sorted ring/vector with bounded capacity.

Prefer deterministic behavior over theoretical generality.

Measure scheduler lateness and expose it in diagnostics:

```text
last_late_us
max_late_us
average_late_us
```

---

# 20. Latency targets

For BYPASS mode:

```text
USB MIDI received → first UART MIDI byte queued:
target < 3 ms typical
hard acceptance < 8 ms under normal load
```

For Block chord:

```text
input Note On → generated chord scheduling:
target < 4 ms
```

Jitter for rhythmic events:

```text
target < 1 ms typical
acceptable < 2 ms
```

UI rendering must never delay MIDI.

Do not redraw the entire LCD on every incoming MIDI message.

---

# 21. Tasking / concurrency

A reasonable FreeRTOS split:

```text
USB Host task
    ↓ queue
MIDI Engine task
    ↓ scheduled events
Scheduler / MIDI Out task

UI/Input task runs independently
```

Suggested priorities:

1. MIDI output scheduler: highest application priority.
2. USB MIDI servicing.
3. MIDI transformation engine.
4. Cardputer key scanning/UI.
5. storage/logging.

Do not write SD synchronously in a timing-critical path.

Inter-task queues must have bounded capacity and overflow diagnostics.

---


# 22. Physical control model

The Cardputer keyboard and the connected MIDI controller are both first-class control surfaces. All hardware inputs must dispatch into the same semantic action layer.

```text
Cardputer key ─┐
SMK pad ───────┼──► semantic action ─► engine state
Other MIDI ────┘
```

## 22.1 Eight direct chord controls

Cardputer mapping:

```text
       CHORD TYPE
 Q        W        E        R
DIM      MIN      MAJ      SUS

       EXTENSIONS
 A        S        D        F
 6       m7       M7        9
```

SMK-37 canonical mapping:

```text
PAD1     PAD2     PAD3     PAD4
 DIM      MIN      MAJ      SUS

PAD5     PAD6     PAD7     PAD8
  6       m7       M7        9
```

These are two front ends to the same commands.

## 22.2 Core Cardputer controls

```text
[        = voicing down
]        = voicing up

K        = key/key-mode interaction
P        = performance
B        = bass
L        = loop
T        = tap tempo
C        = clock toggle
V        = cycle display view
H        = harmonic quantize

Arrow L/R = select contextual parameter
Arrow U/D = change contextual parameter
Enter     = confirm / enter
Esc       = back / dismiss overlay
```

Exact keycodes may change after physical ergonomics testing; preserve semantic intent.

## 22.3 Panic

Preferred Cardputer gesture:

```text
Fn + Esc = PANIC
```

Fallback: long-press Esc.

A controller profile may also map any external button/pad to `panic`.

## 22.4 Presets

Suggested:

```text
Fn + S = save preset
Fn + L = load preset
```

Initial presets may be numbered; do not prioritize text entry over musical functionality.

---

# 23. UI / UX design — Orchid-style instrument interface

Resolution: `240×135`.

The normal UI should feel like a musical instrument, not a MIDI diagnostics tool.

Core design rule:

> **Physical actions do the work; the display explains the current musical state.**

The dominant element in ordinary use is the current chord/harmonic identity. Technical detail appears contextually or in Diagnostics.

## 23.1 Three UI layers

### Layer 1 — Performance view

Used approximately 95% of the time.

Example:

```text
┌──────────────────────────────────────┐
│ USB ●                         MIDI ● │
│                                      │
│               Cmaj7                  │
│                                      │
│ C MAJOR                   VOICE +2   │
└──────────────────────────────────────┘
```

Do not permanently show VID/PID, queues, scheduler latency, file paths or raw packet counters here.

### Layer 2 — Context overlay

Manipulating a control temporarily replaces the center of the display with that parameter.

Example after selecting Performance:

```text
┌──────────────────────────────────────┐
│                                      │
│             PERFORMANCE              │
│                                      │
│               ARP ↑                  │
│                                      │
│               1/16                   │
└──────────────────────────────────────┘
```

Adjusting rate:

```text
┌──────────────────────────────────────┐
│                                      │
│              ARP RATE                │
│                                      │
│                1/16                  │
│                                      │
│       ◀  1/8    1/16    1/32  ▶     │
└──────────────────────────────────────┘
```

After roughly `750–1200 ms` of no related input, return automatically to Performance view.

### Layer 3 — Options

Only low-frequency configuration belongs here:

```text
OPTIONS
> Controller
  MIDI Routing
  Input
  Presets
  Loop
  Display
  System
  Diagnostics
```

Do not require Options for normal performance operations.

## 23.2 Chord linger

When the final held root is released, keep the last chord name visible for about `1200 ms`, then return to the idle/key-state screen.

## 23.3 Display views

`V` cycles:

```text
CHORD
NOTES
KEYBOARD
GEEK
```

### CHORD

Sparse hero chord display.

### NOTES

Example:

```text
Cmaj9

C3  E3  G3  B3  D4
```

### KEYBOARD

Draw a compact piano keyboard and mark active generated pitches. Do not rely on color alone.

### GEEK

Information-rich live view:

```text
┌──────────────────────────────────────┐
│ Cmaj9                       V:+2     │
│ C3 E3 G3 B3 D4                       │
│                                      │
│ [small keyboard visualization]       │
│                                      │
│ ARP ↑    1/16       104 BPM   CH1   │
└──────────────────────────────────────┘
```

Geek view is the correct place for BPM, division, output channel and generated-note detail.

## 23.4 Voicing overlay

Changing voicing gets a dedicated visualization:

```text
             VOICING +3

 C3      E3      G3      B3      D4
 ●───────●───────●───────●───────●
                         ▲
                       SPLIT
```

If a note wraps by an octave, briefly emphasize only that movement. Animation is cosmetic and asynchronous; MIDI timing must never wait for it.

## 23.5 Chord-control feedback

Pressing Q/W/E/R, A/S/D/F or the mapped SMK pad should produce the same short overlay.

Example:

```text
                MAJOR

             + M7  + 9
```

Toggling an extension may briefly show `M7 ON` or `M7 OFF`.

## 23.6 Key selection from the external keyboard

Preferred interaction:

```text
hold/long-press K
```

Display:

```text
SELECT KEY

Play root note…
```

Then a note played on the SMK-37 sets the key root. Major/minor/modal scale selection remains a Cardputer or mapped-controller action.

## 23.7 External-controller UI equivalence

The UI must not care which physical surface invoked the action.

```text
SMK PAD 3
    ↓
chord.maj
    ↓
MAJOR overlay
```

Cardputer E and mapped PAD 3 therefore produce identical engine and display behavior.

## 23.8 Controller profile screen

Add:

```text
CONTROLLER

Device:
M-VAVE SMK-37

Profile:
SMK37

> Learn Control
  Set Up 8 Chord Pads
  Mapping List
  Select Profile
  Reload Profiles
```

Mapping list example:

```text
1  Note ?? Ch?? → DIM
2  Note ?? Ch?? → MIN
3  Note ?? Ch?? → MAJ
4  Note ?? Ch?? → SUS
...
```

Do not display invented SMK note numbers before they are learned from hardware.

## 23.9 Eight-pad setup wizard

Dedicated wizard:

```text
1/8
PRESS PAD FOR:
DIM
```

then MIN, MAJ, SUS, 6, m7, M7, 9.

After each input, show the learned MIDI selector and action. On completion offer atomic save to `SMK37.JSON`.

## 23.10 Generic MIDI Learn UI

```text
LEARN CONTROL

Press or move a
MIDI control…
```

On receive:

```text
MIDI RECEIVED
CC 21 / Ch 1
Value 127

Map to:
> VOICING UP
```

For continuous controls allow selection between absolute `value` and supported relative-encoder modes.

## 23.11 Connection indicators

Keep connection state subtle:

```text
USB ●   MIDI ●   SD ●
```

Filled = ready, outline = absent, `!` = error.

## 23.12 Idle display

Examples:

```text
              C MAJOR
              KEY MODE

ARP ↑                       104 BPM
```

or:

```text
              BYPASS
```

or:

```text
          MAJOR + M7 + 9
```

## 23.13 Rendering constraints

UI runs below MIDI priority.

Rules:

- no full-screen redraw for every MIDI event;
- use dirty regions/state transitions;
- overlays use timestamps/deadlines, never `delay()`;
- no SD access from render path;
- no diagnostics formatting in MIDI hot path;
- cap Performance UI around 30 Hz;
- diagnostics may refresh at 5–10 Hz.

## 23.14 Visual language

Take inspiration from Orchid's interaction hierarchy, not its branding.

Desired:

- large musical labels;
- high contrast;
- minimal decoration;
- generous empty space;
- contextual information;
- immediate return to the musical state after adjustment.

Do not copy Orchid logos, product names, proprietary artwork or pixel-perfect screen skins.

Design principle:

```text
direct control → contextual feedback → return to music
```

---

# 24. USB MIDI host behavior

USB is the highest hardware risk and must be built first.

## 24.1 Enumeration requirements

On attachment:

1. detect device;
2. retrieve descriptor information;
3. enumerate configurations/interfaces;
4. identify MIDI Streaming interface;
5. identify inbound endpoint;
6. claim interface;
7. start asynchronous receive;
8. update UI to connected;
9. log all relevant descriptor information.

On removal:

1. stop transfers;
2. release interface;
3. call Panic;
4. clear connection state;
5. return to wait state.

## 24.2 Composite device requirement

The code must tolerate:

```text
USB Device
├── Audio Control
├── Audio Streaming
└── MIDI Streaming
```

It must not attempt to initialize USB audio.

If multiple MIDI interfaces/cables exist, use cable 0 initially and record that limitation.

## 24.3 Debugging

Because the native USB connector is busy acting as host, ordinary USB serial logging may not be available during the test.

Provide at least two debug mechanisms:

1. on-device diagnostics screen;
2. optional log ring buffer export to microSD.

Optional development feature:

- Wi-Fi UDP/TCP log viewer.

Wi-Fi must be disabled by default in release firmware.

---

# 25. UART / DIN MIDI output

Implement a raw MIDI serializer.

Required messages:

- Note Off
- Note On
- Poly Aftertouch
- CC
- Program Change
- Channel Pressure
- Pitch Bend
- MIDI Clock
- Start
- Continue
- Stop
- basic SysEx forwarding later

For the first release, always emit complete status bytes rather than implementing running status.

This costs negligible bandwidth and simplifies correctness.

At 31,250 baud:

- output bandwidth is finite;
- large generated chords plus dense CC data can congest the port.

The output layer should expose queue depth and dropped-event counters.

Priority rules under overload:

1. Never drop Note Off.
2. Never drop Panic/All Notes Off.
3. Prefer Note On over continuous CC spam.
4. Coalesce high-rate duplicate CC if necessary.
5. Diagnostics may report congestion.

---

# 26. MIDI loop engine

This is not required for the very first USB-to-DIN prototype, but belongs in the full project scope.

Record **MIDI events**, never audio.

Modes:

```text
FREE
1 BAR
2 BAR
4 BAR
8 BAR
16 BAR
```

Features:

- record;
- playback;
- overdub;
- undo latest overdub layer;
- clear;
- stop;
- quantize;
- save;
- load.

Quantize grid:

```text
1/4
1/8
1/8T
1/16
1/16T
1/32
```

A loop stores post-engine performance MIDI by default so playback reproduces what the user heard.

Later option:

```text
record_source = INPUT | GENERATED
```

## Loop note safety

Stopping or clearing playback must release all notes owned by the loop playback instance.

---

# 27. Presets and persistence

Use NVS for global settings.

Use microSD for larger preset/loop libraries if inserted.

Preset must capture at least:

```text
mode
key root
scale
chord quality
extensions
harmonic quantize
voicing step
performance mode
performance direction
rate
gate
strum interval
slop amount
BPM
clock enabled
bass mode
bass octave
performance channel
bass channel
raw chord channel
stream enables
expression routing
input channel filter
input note range
```

Use versioned serialized data:

```json
{
  "schema": 1,
  ...
}
```

Never make a future firmware update unable to boot because an old preset is malformed.

---

# 28. State model

Suggested global state:

```cpp
struct AppState {
    EngineMode mode;

    HarmonicState harmonic;
    PerformanceState performance;
    BassState bass;
    RoutingState routing;
    ClockState clock;

    UsbState usb;
    RuntimeStats stats;
};
```

State changes must be explicit commands/events rather than random UI mutation.

Suggested command model:

```cpp
enum class AppCommand {
    SetMode,
    SetChordQuality,
    ToggleExtension,
    SetKey,
    SetScale,
    ChangeVoicing,
    SetPerformance,
    SetTempo,
    Panic,
    ...
};
```

This makes UI and future external control easier to test.

---

# 29. Chord naming

The UI must generate readable chord labels.

Examples:

```text
C
Cm
Cdim
Csus4
C6
Cm7
Cmaj7
C9
Cm9
Cmaj9
Cm7(add9)
```

Do not let naming logic drive the harmonic logic.

Chord representation is intervals; label rendering is separate.

Use flats/sharps according to key where practical.

MVP may default to sharps.

---

# 30. Expression behavior

## Velocity

Generated chord voices inherit the source Note On velocity.

For multi-octave performance modes, preserve velocity unless Slop/humanize is enabled.

## Pitch bend

Default: broadcast to all currently enabled generated melodic output channels.

## Mod wheel CC1

Default: broadcast to all active melodic streams.

## Sustain CC64

Forward to all active melodic streams.

## Aftertouch

Channel pressure:

- broadcast according to expression-routing setting.

Poly aftertouch:

- BYPASS: forward unchanged.
- Chord mode: default to source/performance channel only; do not invent a mapping onto every generated pitch until explicitly designed.

---

# 31. MIDI input filtering

Settings:

```text
input_channel = OMNI or 1..16
root_input_low = 0..127
root_input_high = 0..127
```

Events outside the root note range:

Default:

```text
pass through unchanged
```

This permits future split behavior:

```text
C1-B2 = chord roots
C3-C6 = normal lead keyboard
```

---

# 32. Revoicing and live parameter changes

Live edits while notes are held are important.

## Change chord quality/extensions while held

Rebuild the generated voice.

Diff old vs new pitches.

- common pitches remain on;
- removed pitches get Note Off;
- added pitches get Note On.

## Change voicing while held

Same diff algorithm.

## Change output channel while notes held

Safest behavior:

1. turn off all old-channel notes;
2. update channel;
3. retrigger currently held generated voices on new channel.

## Change performance mode while held

MVP behavior:

1. stop old performance scheduler events;
2. release active notes owned by old performer;
3. initialize new performer against currently held roots.

Document this behavior.

---

# 33. Development phases

Codex should implement in milestones and keep every milestone buildable.

## Phase 0 — skeleton and host tests

Deliver:

- repository;
- build instructions;
- pure C++ MIDI types;
- chord engine;
- voicing engine;
- host-side unit tests;
- basic Cardputer screen test;
- raw UART MIDI output class.

Acceptance:

- desktop tests pass;
- Cardputer builds/flashes;
- pressing a Cardputer test key can send a known MIDI Note On/Off through Unit MIDI DIN OUT.

## Phase 1 — USB host proof

Deliver:

- USB enumeration;
- descriptor logging;
- MIDI interface claim;
- SMK-37 Note On/Off receive;
- disconnect handling;
- USB diagnostics screen.

Acceptance:

- SMK-37 connects without PC;
- pressing C on SMK-37 is visibly identified on Cardputer;
- release is detected;
- repeated unplug/replug works;
- no crash after 20 reconnect cycles.

## Phase 2 — transparent bridge

Deliver:

- BYPASS mode;
- notes;
- velocity;
- sustain;
- pitch bend;
- modulation;
- CC;
- program change;
- UART DIN output;
- panic.

Acceptance:

```text
SMK-37 → Cardputer → Unit MIDI → external DIN MIDI target
```

behaves like a normal MIDI cable for ordinary playing.

Test for at least 15 minutes with no stuck notes.


## Phase 2.5 — controller profiles and chord pads

Deliver:

- SD JSON controller-profile parser;
- semantic action mapping layer;
- event consumption before musical routing;
- Generic profile fallback;
- SMK-37 eight-pad setup wizard;
- profile selection/reload;
- Cardputer keys and external MIDI controls dispatch through the same action system.

Acceptance:

- learn the real messages from SMK Pads 1–8;
- save `/midi-brain/controllers/smk37.json`;
- reboot/reconnect and reload the same mapping;
- Pads 1–4 control DIM/MIN/MAJ/SUS;
- Pads 5–8 control 6/m7/M7/9;
- consumed control-pad messages do not sound downstream;
- malformed or missing profile safely falls back to Generic.

## Phase 3 — harmonic MVP

Deliver:

- CHORD mode;
- base qualities;
- additive 6/m7/M7/9;
- voicing;
- Cardputer shortcut keys;
- live chord display.

Acceptance example:

Input C3 + Major + M7 + 9:

```text
C3 E3 G3 B3 D4
```

Changing voicing while held does not leave stuck notes.

## Phase 4 — Key mode

Deliver:

- scales;
- diatonic chord construction;
- key selection;
- harmonic quantize;
- key display.

Acceptance:

C major scale generates expected diatonic triads.

## Phase 5 — performance engine

Deliver:

- Block;
- Strum;
- Strum 2 Oct;
- Slop;
- Arp;
- Arp 2 Oct;
- Pattern;
- Harp;
- BPM/division/gate;
- nonblocking scheduler.

Acceptance:

- timing remains stable while UI is being used;
- no blocking waits;
- cancellation on source Note Off works.

## Phase 6 — routing and bass

Deliver:

- Performance/Bass/Raw streams;
- channel selection;
- Bass modes;
- expression broadcast.

## Phase 7 — presets

Deliver:

- NVS settings;
- preset slots;
- load/save;
- schema versioning.

## Phase 8 — loop engine

Deliver:

- MIDI record;
- overdub;
- undo;
- fixed-bar lengths;
- quantize;
- save/load from SD.

## Phase 9 — extended musical features

Optional:

- automatic smooth voice leading;
- user chord recipes;
- Euclidean performance generator;
- probability;
- ratchets;
- CC LFO;
- custom scale editor;
- keyboard splits;
- multiple pattern banks;
- IMU expression;
- MIDI learn.

Do not begin Phase 9 until Phases 1–6 are stable on physical hardware.

---

# 34. Host-side unit tests

Tests must not require ESP32 hardware.

## Chord tests

Examples:

```text
C major       → 60,64,67
C minor       → 60,63,67
C diminished  → 60,63,66
C sus4        → 60,65,67
C maj7        → 60,64,67,71
C 9           → 60,64,67,74
Cm7           → 60,63,67,70
Cm9           → 60,63,67,70,74
```

## Voicing tests

```text
[60,64,67], +1 → [64,67,72]
+2 → [67,72,76]
-1 → [55,60,64] or equivalent defined inverse behavior
```

Once behavior is chosen, pin exact expectations.

## Key tests

C major:

```text
C → major
D → minor
E → minor
F → major
G → major
A → minor
B → diminished
```

## Lifecycle tests

- repeated Note On same pitch;
- overlapping same source pitch generations;
- Note On velocity 0;
- chord change while held;
- voicing change while held;
- USB disconnect;
- output channel change;
- Panic;
- arp cancelled on release;
- strum cancelled before all delayed notes fire.

The invariant after every test:

```text
active_note_registry.empty() == true
```

once all sources have been released/panicked.

## Scheduler tests

- stable event ordering;
- cancellation by VoiceId;
- due-event polling;
- no cancelled Note On emitted after source release;
- Note Off priority.

---

# 35. Hardware test checklist

## Unit MIDI test

1. Set Unit MIDI switch to SEPARATE.
2. Connect Grove cable.
3. Connect Unit DIN OUT to known MIDI receiver.
4. Send C4 Note On channel 1 velocity 100.
5. Wait 300 ms using a non-production smoke test.
6. Send C4 Note Off.
7. Confirm external receiver responds.

If this fails, do not proceed to USB.

## SMK-37 USB test

1. Fully charge SMK-37.
2. Boot Cardputer.
3. Enter USB diagnostics.
4. Connect SMK-37 via known-good USB data path.
5. Confirm VID/PID/product.
6. Confirm MIDI interface is claimed.
7. Press/release keys.
8. Move pitch/mod controls.
9. Press sustain.
10. Exercise pads/knobs/faders.
11. Disconnect/reconnect repeatedly.

Store observed descriptor information in `docs/SMK37_TEST_RESULTS.md`.

## Bridge test

Connect:

```text
SMK-37 → Cardputer → Unit MIDI → external MIDI receiver
```

Test:

- chromatic scale;
- rapid repeated notes;
- 10-note clusters;
- sustain down/up;
- pitch bend;
- mod;
- knobs;
- panic;
- USB disconnect during a sustained chord.

No stuck notes are acceptable.

---

# 36. Diagnostics counters

Track at minimum:

```text
usb_connect_count
usb_disconnect_count
usb_packets_rx
midi_events_rx
midi_events_generated
midi_events_tx
midi_events_dropped
uart_queue_high_water
scheduler_queue_high_water
scheduler_late_count
scheduler_max_late_us
active_voice_count
active_note_count
panic_count
```

Make these viewable on device.

---

# 37. Error handling

The application must remain usable after recoverable faults.

## USB device disappears

- panic;
- clear USB state;
- remain running;
- wait for reconnect.

## USB enumeration unsupported

Show:

```text
USB DEVICE FOUND
NO MIDI STREAMING INTERFACE
```

Do not crash.

## MIDI output queue full

- prioritize Note Off / panic messages;
- increment dropped count;
- show warning icon;
- recover automatically.

## SD missing

Only disable SD-dependent preset/loop storage.

Core MIDI functionality must continue.

## malformed settings

Fall back to defaults and preserve a diagnostic message.

---

# 38. Performance constraints

Do not:

- dynamically allocate in the MIDI hot path;
- use String concatenation in event processing;
- redraw LCD synchronously for every note;
- log every event to SD synchronously;
- use `delay()` in arps/strums;
- calculate expensive UI layout at MIDI priority.

Prefer:

- fixed arrays;
- ring buffers;
- precomputed scale tables;
- preallocated voices;
- static pattern tables;
- dirty-region UI refresh;
- bounded queues.

Target at least:

```text
16 simultaneous source voices
16 generated notes per voice worst-case design capacity
```

Even if ordinary musical use is far below this.

---

# 39. Security/network policy

No network connection is needed in release operation.

If Wi-Fi logging is added for development:

- compile-time or settings toggle;
- disabled by default;
- no hardcoded personal Wi-Fi credentials in repository;
- credentials never committed.

---

# 40. Suggested default configuration

```text
Mode:                BYPASS on very first boot
Chord quality:       MAJOR
Extensions:          none
Key:                 C
Scale:               Major
Harmonic quantize:   off
Voicing:             0

Performance:         BLOCK
Direction:           UP
BPM:                 100
Division:            1/16
Gate:                75%
Strum interval:      25 ms
Slop:                20%

Performance stream:  ON, MIDI Ch 1
Bass stream:         OFF, MIDI Ch 2
Raw chord stream:    OFF, MIDI Ch 3

Input channel:       OMNI
Input range:         0..127
Expression routing:  ALL ACTIVE GENERATED CHANNELS
MIDI clock out:      OFF
```

BYPASS is the safest first-boot mode because it proves the physical bridge before transformations are enabled.

---

# 41. Definition of MVP done

The MVP is done when all of the following are true:

- Cardputer boots standalone.
- M5 Unit MIDI is driven on GPIO2 TX at 31,250 baud.
- SMK-37 enumerates as USB MIDI host input.
- Composite USB interfaces do not break enumeration.
- SMK-37 keys pass through to external DIN-MIDI hardware in BYPASS mode.
- Velocity works.
- Sustain works.
- Pitch bend works.
- Mod wheel works.
- USB disconnect triggers panic.
- No stuck notes during normal play.
- CHORD mode generates Major/Minor/Dim/Sus chords.
- 6/m7/M7/9 can be combined.
- Voicing can be adjusted live.
- KEY mode generates correct diatonic triads.
- Block/Strum/Slop/Arp/Pattern/Harp exist.
- Performance timing is nonblocking.
- Cardputer UI displays current harmony/performance state.
- Performance UI follows the chord-centric/context-overlay hierarchy.
- SMK-37 Pads 1–8 can be learned/mapped to DIM/MIN/MAJ/SUS/6/m7/M7/9.
- Mapped pads invoke the same semantic action path as Cardputer chord keys.
- Consumed control messages do not leak to DIN MIDI output.
- Controller mappings load from SD JSON profiles.
- Unknown controllers fall back to Generic rather than failing.
- Panic is globally available.
- Preset state survives reboot.
- Engine unit tests run on a desktop host.

The looper is part of the larger product scope but is not required to call the harmonic MIDI brain MVP functional.

---

# 42. Definition of full v1 done

Full v1 additionally requires:

- bass stream;
- raw chord stream;
- independent MIDI channel routing;
- MIDI clock output;
- tap tempo;
- fixed-length MIDI loops;
- overdub;
- undo latest overdub;
- quantize;
- SD save/load;
- robust diagnostics;
- graceful queue-overflow behavior;
- 30-minute stress test without stuck notes or crash;
- 20 USB reconnect cycles;
- documented exact toolchain/dependency versions;
- generic MIDI Learn workflow;
- select/reload custom controller profiles from SD;
- malformed-profile recovery tested;
- at least one non-SMK class-compliant controller can be mapped without recompiling;
- `docs/SMK37_TEST_RESULTS.md` populated from real hardware.

---

# 43. README requirements

The repository README must explain:

1. What the device does.
2. Explicitly that it produces **no audio**.
3. Hardware required.
4. Wiring.
5. Unit MIDI must be in SEPARATE mode.
6. How to build.
7. How to flash.
8. How to enter Cardputer ADV download mode.
9. Initial SMK-37 connection procedure.
10. Powered USB hub troubleshooting.
11. Cardputer keyboard shortcuts.
12. Panic gesture.
13. MIDI channel defaults.
14. Known limitations.
15. Controller profile folder and JSON format.
16. SMK-37 eight-pad setup/calibration.
17. Generic MIDI Learn for custom controllers.

Include this wiring diagram:

```text
SMK-37 USB-C
     │
     ▼
Cardputer ADV USB-C (HOST)
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

---

# 44. Acceptance scenario

A canonical live-use test:

1. Power Cardputer.
2. Unit MIDI connected in SEPARATE mode.
3. Connect charged SMK-37 over USB.
4. Cardputer indicates `USB ●`.
5. External synth/processor connected to DIN MIDI OUT.
6. Select CHORD.
7. Press SMK-37 PAD 3, mapped to MAJ.
8. Press SMK-37 PAD 7 and PAD 8 to enable M7 and 9.
9. Press C on SMK-37.
10. External receiver gets:
    - C
    - E
    - G
    - B
    - D above octave.
11. Press `]` on Cardputer.
12. Voicing shifts upward while key remains held without stuck notes.
13. Select ARP.
14. Chord is arpeggiated at master BPM.
15. Move SMK modulation control.
16. Modulation is received downstream.
17. Release C.
18. Every generated note ends.
19. Unplug SMK-37 during another held chord.
20. Cardputer performs panic automatically.
21. Replug SMK-37.
22. Operation resumes without reboot.

If this scenario is reliable, the architectural core is correct.

---

# 45. Future enhancements — explicitly after v1

Once the core bridge is reliable, possible differentiators beyond the Orchid-style workflow:

## Generative performance
- Euclidean rhythms.
- Probability per note.
- Ratchets.
- note omission.
- octave probability.
- mutation amount.

## Voice-leading intelligence
- nearest-inversion search;
- common-tone preservation;
- maximum-motion constraint;
- drop-2/drop-3 voicings.

## MIDI modulation
- internal LFO → CC;
- multiple LFO shapes;
- tempo sync;
- random/sample-and-hold;
- per-stream destinations.

## Controller mapping
- MIDI learn from SMK-37 knobs/faders;
- map encoder to voicing;
- map fader to strum/slop;
- map pad to performance mode;
- map pedal to latch.

## IMU
Optional Cardputer tilt can control:
- voicing;
- strum speed;
- CC;
- arp density.

IMU must never be required for normal operation.

## Scale/chord authoring
- user scales;
- user chord recipes;
- microtonal support only if later needed.

---

# 46. References used when scoping this build

These are engineering references, not runtime dependencies.

- M5Stack Cardputer ADV documentation:
  https://docs.m5stack.com/en/core/Cardputer-Adv

- M5Stack Unit MIDI hardware documentation:
  https://docs.m5stack.com/en/unit/Unit-MIDI

- M5Stack Unit MIDI Arduino tutorial showing standard 31,250-baud UART:
  https://docs.m5stack.com/en/arduino/projects/unit/unit_midi

- M-VAVE SMK-37 Pro product page:
  https://www.m-vave.com/product?id=smk-37-pro

- M-VAVE download/support area:
  https://www.m-vave.com/download

- ESP32 USB MIDI host implementation reference:
  https://github.com/enudenki/esp32-usb-host-midi-library

- ESP32 multi-transport MIDI host implementation reference:
  https://github.com/sauloverissimo/ESP32_Host_MIDI

- ESP-IDF USB host MIDI example proposal/reference:
  https://github.com/espressif/esp-idf/pull/12566

- Cardputer ADV native USB-host firmware proving the hardware path:
  https://github.com/integerQuant/baitnswitch

---

# 47. Final instruction to Codex

Build this incrementally, but optimize for a real hardware test as early as possible.

Do not spend time polishing UI before these four things work:

```text
1. Cardputer → UART → Unit MIDI → DIN OUT
2. SMK-37 → USB host → Cardputer receives MIDI
3. SMK-37 → Cardputer → DIN transparent BYPASS
4. CHORD transformation with correct Note Off lifecycle
5. SMK-37 Pads 1–8 → Orchid-style chord controls through controller profile
```

Once those are proven, continue through the musical engine phases.

Whenever there is a choice between an elegant abstraction and prevention of stuck notes, choose prevention of stuck notes.

Whenever there is a choice between additional features and deterministic timing, choose deterministic timing.

The product is not a synthesizer.

**The product is the MIDI brain between the controller and the instrument.**
