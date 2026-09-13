# MIDI behavior

## Modes

`BYPASS` sends normalized input directly to DIN with channel and values preserved until explicit lane selection is used. After lane selection, channel messages use the selected primary output channel. Note On velocity zero becomes Note Off before routing.

`CHORD` treats each accepted Note On as a separately owned root. Latched style immediately uses the selected chord. Simple requires quality before root; Advanced permits one conversion from a held single note; Free permits repeated quality changes/retriggers. Quality, extensions, quantization, transpose, and voicing produce the pitch set. Source Note Off cancels pending events and releases every generation of that channel/note. Bypass and out-of-filter notes also have registry ownership, protecting them from overlapping loop releases.

Extension controls are momentary by default: press sets the extension bit and release clears it. Stack mode changes press behavior to a persistent toggle and ignores release for extension state. Switching between momentary and stack modes clears every extension and safely rebuilds held voices. Momentary extension state is not persisted; stack mode and its active extension set are persisted.

`KEY` stacks scale degrees for the base triad, then adds explicit extension intervals relative to the computed chord root: 6 = 9 semitones, m7 = 10, M7 = 11, and 9 = 14. Extensions remain independent and may fall outside the scale; KEY's automatic branch does not re-quantize them. A held quality selects the manual chord branch, where harmonic quantization can snap the completed chord. Extreme pitches fold by octaves to remain within MIDI bounds and duplicate pitches are removed. Available scales are Major, natural minor, Dorian, Mixolydian, harmonic minor, major pentatonic, minor pentatonic, chromatic, and 12-tone approximations of Hijaz, Hijazkar, Kurd, and Nikriz; no quarter-tone output is implied.

## Lifecycle

Every generated note is keyed by source channel, source note, generation, stream, output channel, and pitch. Repeated source notes receive different generations. Physical pitches are reference counted across owners, so releasing one overlapping voice cannot terminate another.

Held block-chord edits reconcile desired notes against the registry. Common tones remain active, removed tones receive Note Off, and new tones receive Note On. Timed modes cancel their old owner events, release emitted pitches, and initialize the new performer.

Root release cancels Note Ons that have not fired. Harp Note Off events and arp gates are also canceled before the registry explicitly releases anything already emitted.

## Performance timing

The live keyboard observes successfully accepted sink messages after scheduler dispatch and registry deduplication. Its bounded output gate bitmap and recent-attack ring allocate no memory and do not render or access storage. The observer does not generate or reschedule notes. Its timestamps use the later of the event time and current monotonic dispatch time, so delayed events are not shown as earlier attacks. Rejected sends do not light keys; panic clears visual state. DIN queue acceptance does not prove physical transmission or receiving-synth sound state.

The scheduler is a fixed 512-event stable min-heap. Equal timestamps preserve insertion order. Strum defaults to 25 ms. Harp defaults to 18 ms spacing and 350 ms overlap. Slop uses a seeded xorshift generator and clamps timing to the initiating timestamp. Arp duration derives from integer microseconds and never accumulates a floating-point phase error.

Arp and harp reserve capacity for Note On/Off pairs. After a long stall, expired arp steps and clocks are skipped rather than emitted as an unbounded catch-up burst. Physical timing limits still require measurement.

## Loop ownership and persistence

The loop records post-engine channel events, converting Note On/Off pairs into onset/duration entries. MIDI-clock/system events are not recorded. Playback uses `StreamId::Loop`, distinct owners, the same scheduler, and the same physical-pitch registry as live playing. Playback is excluded from recording feedback. Stop, clear, undo, disconnect, and panic clean up playback ownership. Sustain reset is limited to loop-used sustain channels except for a global panic.

Fixed-length loops auto-play at the boundary; free loops close on user action. Overdub entries carry layer IDs; undo removes the latest layer and stops playback. Capacity is 512 entries and 128 pending notes; grid is 96 ticks per quarter with optional onset quantization. Durations crossing the cycle end are clipped. Recorded BPM/channels are retained; this is not an external-clock-following or time-stretch engine.

Profiles, presets, loops, NVS settings and diagnostic export use an idle storage service, not MIDI callbacks. JSON values are validated before narrowing and malformed loads do not partially apply a preset/loop. Profile failure uses an empty Generic mapping. SD saves use temporary/backup replacement; filesystem power-loss durability must be tested physically.

## Panic

Panic clears scheduler events, sends CC64, CC120, CC121, and CC123 on channels 1–16, explicitly emits Note Off for every remaining physical pitch, clears voices, and increments diagnostics. USB disconnect, BLE disconnect, and output-lane changes invoke panic.

DIN queue saturation prioritizes critical releases. If no safe slot remains for a critical message, a recovery flag makes the next main-loop pass discard queued traffic and issue a complete panic. BLE overflow discards its incomplete event batch before invoking panic; disconnect discards stale queued input before new connection events are accepted. This preserves a safe recovery path under overload, not unlimited lossless throughput.
