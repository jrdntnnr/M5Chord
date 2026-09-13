# Retro instrument display

## Brief — 2026-09-13

The player needs to recognize the current harmony and see generated note timing without reading instructions during performance. The user's IMG_2280.HEIC shows that the previous design gives too much space to explanatory text, duplicated keyboard labels, and metadata.

Direction: late-1970s shipboard instrumentation, loosely in the world of Alien (1979), not a replica. Black ground, phosphor green lettering, amber only for latched state/attention, square rules and quiet instrument labels. No scanline filter, decorative animation, rounded cards, or prose on the performance page.

Typography uses the pinned renderer's local Orbitron display face and compact terminal glyphs. The chord receives the largest region; scale, mode and routing become peripheral information. Explicit quality and extension feedback remains available before playing. Options preserves its navigation and contextual help, with a larger selected value and clearer separation. No bindings change.

The actual 240×135 M5GFX framebuffer is the visual test surface, not a browser or a generated bitmap concept. This existing embedded design system and its memory/font constraints dictate code-native assets. Keep the 8-bit framebuffer and avoid additional rendering work in MIDI callbacks.

## Live keyboard

Observe accepted output messages after scheduler/ownership processing, not the input root or cached chord. Show the selected performance MIDI channel, including loop messages on that channel; transparent BYPASS displays all output channels. Other stream channels do not falsely light the performance keyboard.

Held output gates are green; a short attack marker preserves visibility for gates shorter than a display frame. Attacks use actual dispatch timestamps, never a deferred visual replay queue. Releasing a root, panic, disconnect and routing changes clear active state correctly. The observer is portable, bounded and allocation-free. UI reads snapshots; it never delays MIDI to animate a key. LCD frame cadence cannot resolve arbitrarily close events or establish physical DIN/audio latency.

## Acceptance

- Large short and extended chord names; no instructional performance text.
- Whitespace between header, chord/keyboard, modifier rail and transport footer.
- Readable small labels; no DejaVu face or duplicated pad shortcut labels.
- Options values, long mapping names, learning and transient status fit.
- Arp, strum, slop, pattern, harp, block, loop and bypass follow dispatched output; tests cover releases, overlap, short gates, failed sends and panic.
- Strict native tests, sanitizers, pinned firmware build and actual-renderer still/sequence previews pass before handoff. Physical setup/results belong in SMK37_TEST_RESULTS.md.
