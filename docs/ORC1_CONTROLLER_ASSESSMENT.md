# ORC-1 controller assessment

Assessed against Telepathic Instruments' public Orchid documentation on 2026-09-12 and this repository's controller expansion. This is a source/code comparison, not a physical side-by-side test. No firmware was flashed during the expansion.

## Verdict

The build covers the main Orchid-inspired harmonic-controller workflow and the previously deferred software roadmap: control editing, profiles, presets, MIDI looping, and diagnostic export. It is not an exact ORC-1 clone. Public behavior guides informed the interaction; proprietary voicing and pattern tables were not copied or inferred as facts. Synth/audio features are intentionally excluded.

## Controller comparison

| Area | This build | Parity boundary |
|---|---|---|
| Chord qualities and extensions | Major/minor/diminished/suspended, 6/m7/M7/9 and combinations; momentary or stacked extensions | Covers the basic harmonic palette; no secret-chord combination table. [Extensions](https://support.telepathicinstruments.com/hc/en-us/articles/16576229505167-Chord-Extensions-Explained) |
| Playstyles | Simple requires quality before root; Advanced converts a held single note once; Free allows repeated quality changes/retriggers | Implements the documented broad interaction, not undocumented edge cases. Extra Latched style preserves this project's tap-quality workflow. [Playstyles](https://support.telepathicinstruments.com/hc/en-us/articles/15280843614863-What-s-the-Difference-Between-Playstyles) |
| Extension addition | Add only changed tones or retrigger the chord | Block-mode common tones are preserved; timed modes restart after harmony changes. [Options](https://support.telepathicinstruments.com/hc/en-us/articles/15280847893263-Navigating-the-Options-Menu-A-Complete-Overview) |
| Key mode | Key root learning, twelve scales, scale-derived triads with independent literal 6/m7/M7/9 additions, manual-quality override and harmonic quantization | Same general musical purpose, with our own scale/voicing rules; explicit extensions may leave the scale. [Key mode](https://support.telepathicinstruments.com/hc/en-us/articles/15280829123087-How-to-Use-Key-Mode-for-Easy-Chord-Progressions) |
| Voicing | Move lowest tone up/highest tone down with bounded inversion steps; transpose | No claim to Orchid's exact starting voicings, melodic behavior, or proprietary tables. [Voicing](https://support.telepathicinstruments.com/hc/en-us/articles/15292199149839-Voicing-Engine-and-Inversions) |
| Performance | Block, Strum/2 octaves, Slop, Arp/2 octaves, Pattern, Harp; tempo/rate/gate/direction controls | Functional equivalents, not identical algorithms. Pattern currently has one deterministic sequence, not Orchid's preset variety. [Performance modes](https://support.telepathicinstruments.com/hc/en-us/articles/15280943220367-Exploring-Performance-Modes-Arp-Strum-Pattern-and-More) |
| Three MIDI streams | Independent performance, bass and raw-chord channels/enables | Same separation concept; our output is DIN through Unit MIDI. [Three-channel recording](https://support.telepathicinstruments.com/hc/en-us/articles/15303089658127-How-to-Record-MIDI-from-All-Three-Channels-Performance-Bass-Chord) |
| Bass control | Off, harmonic root, lowest voiced note, or source-note unison; octave/channel; solo via disabling other streams | Not an exact recreation of Orchid's chord-only/single-note/solo menu policies. No bass synth. [Options](https://support.telepathicinstruments.com/hc/en-us/articles/15280847893263-Navigating-the-Options-Menu-A-Complete-Overview) |
| Velocity | Source velocity or fixed 100 in generated modes; bypass preserves source | MIDI-only equivalent of sensitivity on/off. [Options](https://support.telepathicinstruments.com/hc/en-us/articles/15280847893263-Navigating-the-Options-Menu-A-Complete-Overview) |
| Looper | Free/fixed bars, record/play/stop, overdub, latest-layer undo, clear, onset quantization, SD save/load | No audio capture or count-in; 512 entries, recorded BPM/channels retained, cross-boundary notes clipped. [Orchid looping](https://support.telepathicinstruments.com/hc/en-us/articles/16646345650447-How-to-make-and-save-a-Loop-on-Orchid) |
| Presets | 16 controller-configuration slots plus NVS globals; separate loop files | Replaces the useful controller-setting portion of sound presets; no sound engine or sound library. |
| Views | Chord, named notes/octaves, keyboard, diagnostics; parameter values, loop state, Options | No waveform or Orchid-specific joke labels; exact notes remain the authority for ambiguous names. [Orchid notation](https://support.telepathicinstruments.com/hc/en-us/articles/15280936711311-Understanding-Chord-Notations-on-the-Display-JAZZ-WTF-x) |
| Connectivity | USB host input, BLE-MIDI central input, DIN output | Orchid can appear as a USB-MIDI device to a DAW; this build cannot. Its native USB port is assigned to hosting a controller. [USB setup](https://support.telepathicinstruments.com/hc/en-us/articles/15303088810383-Connecting-Orchid-to-Your-Computer-for-Use-in-a-DAW-Initial-Setup) |

## Additions useful for this setup

Arbitrary MIDI Learn/edit/delete, portable controller profiles, exact-modifier Cardputer shortcuts, input range/channel filtering, and `L`/learned-pad channel cycling serve the external SMK-37 → Cardputer → Ambient Zero workflow. Three output streams can address separate receiving parts; lane cycling only changes the primary performance channel, not all three streams together. Configure the other channels deliberately to avoid sending duplicate streams to the same receiving part.

Momentary extensions are the default. Latched chord quality is distinct from stacked extensions: choose an ORC-style playstyle in Options without losing the separate `X` stacking toggle.

## Remaining controller-only differences

The subsequent user-directed retro revision replaces the DejaVu layout with Orbitron/terminal typography, a larger chord field and a single modifier rail. The keyboard now follows dispatched output gates and recent attacks instead of the cached full chord, including arp/strum/pattern/harp timing. This is a controller-feedback improvement, not new ORC-1 musical parity. See [retro design and acceptance](RETRO_DISPLAY_DESIGN.md).

The 2026-09-13 revision addresses independent KEY extensions and interface clarity, not proprietary Orchid voicing parity. The display now separates mode, scale application, selected versus held quality, and momentary versus toggled extensions. Options adds neighboring rows and contextual help; all screens use bundled DejaVu Sans. This follows visibility-of-state and recognition-over-recall guidance in [Nielsen Norman Group's usability heuristics](https://www.nngroup.com/articles/ten-usability-heuristics/), with text and shape alongside color as recommended in their [visual indicators guidance](https://www.nngroup.com/articles/visual-indicators-differentiators/). Readability was checked with the actual M5GFX renderer at 240×135; physical usability remains a separate acceptance gate.

- A factory-style pattern library and pattern selector.
- Exact voicing/secret-chord behavior, if desired and independently specified.
- Count-in, phase-preserving loop tempo changes, and external MIDI-clock following.
- Dedicated bass trigger policies and a split-single-note keyboard layout. A full external keyboard already supplies a broader note range.
- USB-MIDI device output or BLE output, requiring transport-role work rather than a menu option.
- Richer preset names, loop progress UI, and battery/power management.

These are additional ORC-parity/product refinements, not a claim that the current controller build implements every Orchid feature. The immediate next gate is physical acceptance of the expanded firmware after flashing is authorized: keyboard rollover, SD persistence/recovery, loop release safety, BLE reconnect, DIN congestion, and measured latency.

## Explicitly excluded

Oscillators, internal synth voices, sound design, audio recording, effects, drum/beat sounds, audio metronome, speaker/headphone routing, waveform rendering, and sound-preset exchange. None is required to control external MIDI instruments.
