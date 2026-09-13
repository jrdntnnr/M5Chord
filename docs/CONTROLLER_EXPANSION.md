# Controller expansion

The expansion was built/tested with flashing on hold. The user subsequently authorized an upload, which passed flash hash verification on 2026-09-12; see `SMK37_TEST_RESULTS.md`. Runtime acceptance remains pending.

## Implementation order

1. Central keyboard bindings with exact modifier matching, key edges, and release ownership; portable conflict tests.
2. Controller options, Simple/Advanced/Free plus compatible Latched interaction, extension retrigger policy, key learning, input and stream routing.
3. General MIDI Learn, mapping editing/deletion, controller profile selection/reload, portable profile validation, deferred storage.
4. Versioned preset slots and bounded post-engine MIDI loops with overdub/undo, quantization and SD storage.
5. Diagnostics, congestion recovery, user-facing feedback, updated acceptance checklist and ORC-1 assessment.

## Keyboard policy

Unmodified Q/W/E/R and A/S/D/F retain their harmonic meanings. L remains lane selection. X remains momentary/stack. O remains pad setup. Tab opens Options. Menu navigation uses semicolon (up), period (down), comma (decrease), slash (increase), Enter (activate), and backtick/Esc (back). These keys are menu-only. Fn shortcuts are exact matches and never invoke the unmodified action. Z handles loop record, Space loop play/stop, Fn+Z overdub, Fn+U undo, Fn+Backspace clear. Fn+S saves the selected preset and Fn+L loads it. Fn+K arms key learning. Fn+Esc remains panic in every context.

## Verification

Each behavior is checked by portable tests where possible. Final gates are CMake/CTest and the pinned Cardputer build. Hardware-dependent USB enumeration, latency and stress acceptance remain physical tests. No upload command is permitted in this work session.

Implementation is complete for the five software steps. See `IMPLEMENTATION_PLAN.md` for physical gates and `ORC1_CONTROLLER_ASSESSMENT.md` for remaining controller-parity refinements. No synth or audio work was added.
