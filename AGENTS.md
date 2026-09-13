# Repository guidance

- Keep the musical core portable C++17 and free of Arduino, ESP-IDF, M5Stack, display, storage, and RTOS dependencies.
- Do not add code comments.
- Do not allocate memory, use `String`, access SD, or render UI in MIDI event processing.
- Route every generated note through `VoiceId`, `StreamId`, `MidiScheduler`, and `ActiveNoteRegistry`.
- Preserve Note Off and panic correctness over feature breadth.
- Use monotonic microseconds for musical deadlines. Never add `delay()` for timing.
- Keep controller inputs behind `SemanticAction`; Cardputer and mapped MIDI controls must use the same path.
- Run the CMake/CTest suite and `pio run -e cardputer-adv` after behavior changes.
- Never claim hardware acceptance without recording the physical setup and result in `docs/SMK37_TEST_RESULTS.md`.

