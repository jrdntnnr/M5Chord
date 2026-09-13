#include "TestSupport.h"
#include "engine/ScaleEngine.h"
#include "controller/Parameters.h"
#include "storage/StateCodec.h"
#include "ui/HarmonyDisplay.h"

#include <array>
#include <cstring>

using namespace midibrain;

void testScales() {
    ScaleEngine engine;
    const std::array<std::array<uint8_t, 3>, 7> expected{{
        {{60, 64, 67}},
        {{62, 65, 69}},
        {{64, 67, 71}},
        {{65, 69, 72}},
        {{67, 71, 74}},
        {{69, 72, 76}},
        {{71, 74, 77}}
    }};
    for (std::size_t degree = 0; degree < expected.size(); ++degree) {
        const auto notes = engine.buildDiatonic(expected[degree][0], 0, ScaleType::Major);
        EXPECT(notes.size() == 3);
        for (std::size_t i = 0; i < 3; ++i) EXPECT(notes[i] == expected[degree][i]);
    }
    NoteList chord;
    chord.push_back(60);
    chord.push_back(63);
    chord.push_back(67);
    const auto quantized = engine.quantizeChord(chord, 0, ScaleType::Major);
    EXPECT(quantized[0] == 60 && quantized[1] == 62 && quantized[2] == 67);

    constexpr std::array<std::array<uint8_t, 7>, 4> arabic{{
        {{0, 1, 4, 5, 7, 8, 10}},
        {{0, 1, 4, 5, 7, 8, 11}},
        {{0, 1, 3, 5, 7, 8, 10}},
        {{0, 2, 3, 6, 7, 9, 10}}
    }};
    EXPECT(static_cast<uint8_t>(ScaleType::Chromatic) == 7);
    for (unsigned i = 0; i < arabic.size(); ++i) {
        const auto scale = static_cast<ScaleType>(static_cast<uint8_t>(ScaleType::Hijaz) + i);
        const auto& definition = engine.definition(scale);
        EXPECT(definition.length == 7);
        for (unsigned degree = 0; degree < 7; ++degree) EXPECT(definition.semitones[degree] == arabic[i][degree]);
        const auto snapped = engine.quantizeChord(chord, 0, scale);
        for (const auto pitch : snapped) EXPECT(engine.contains(0, scale, pitch));
        for (uint8_t key = 0; key < 12; ++key) {
            for (unsigned note = 0; note < 128; ++note) {
                bool expectedMember = false;
                for (const auto interval : arabic[i]) expectedMember = expectedMember || note % 12 == (key + interval) % 12;
                EXPECT(engine.contains(key, scale, note) == expectedMember);
                const auto built = engine.buildDiatonic(note, key, scale);
                EXPECT(!built.empty());
                for (const auto pitch : built) EXPECT(engine.contains(key, scale, pitch));
            }
        }
    }
    EXPECT(std::strcmp(engine.definition(ScaleType::Count).name, "Major") == 0);
    EXPECT(std::strcmp(engine.definition(static_cast<ScaleType>(255)).name, "Major") == 0);

    test::FakeSink sink;
    App app(sink);
    app.state().mode = EngineMode::Key;
    app.apply({SemanticAction::ScalePrev, 1, true}, 1);
    EXPECT(app.state().harmonic.scale == ScaleType::Nikriz);
    adjustParameter(app, 5, 1, 2);
    EXPECT(app.state().harmonic.scale == ScaleType::Major);
    adjustParameter(app, 5, -1, 3);
    EXPECT(app.state().harmonic.scale == ScaleType::Nikriz);
    EXPECT(parameters[5].maximum == ScaleCount - 1);
    for (uint8_t i = 0; i < ScaleCount; ++i) {
        app.state().harmonic.scale = static_cast<ScaleType>(i);
        JsonDocument document;
        StateCodec::encode(app.state(), document);
        AppState restored;
        EXPECT(StateCodec::decode(document, restored));
        EXPECT(restored.harmonic.scale == app.state().harmonic.scale);
        document["scale"] = ScaleCount;
        EXPECT(!StateCodec::decode(document, restored));
        document["scale"] = -1;
        EXPECT(!StateCodec::decode(document, restored));
    }
    app.state().harmonic.scale = ScaleType::Hijaz;
    HarmonyDisplay display;
    display.update(app, 100);
    EXPECT(std::strcmp(display.keyLabel(), "KEY: C Hijaz 12T") == 0);
    app.receive(MidiEvent::noteOn(0, 61, 100, 200));
    app.tick(200);
    EXPECT(app.lastChord().size() == 3);
    EXPECT(app.lastChord()[0] == 61 && app.lastChord()[1] == 65 && app.lastChord()[2] == 68);
    for (uint8_t i = 8; i < ScaleCount; ++i) {
        app.state().harmonic.scale = static_cast<ScaleType>(i);
        app.apply({SemanticAction::ScaleNext, 1, true}, 300 + i);
    }
    app.receive(MidiEvent::noteOff(0, 61, 0, 400));
    app.tick(400);
    EXPECT(app.activeNotes().size() == 0);
    EXPECT(app.scheduler().empty());
    app.state().harmonic.scale = ScaleType::Nikriz;
    app.state().performance.mode = PerformanceMode::Arp;
    sink.events.clear();
    app.receive(MidiEvent::noteOn(0, 62, 100, 1000000));
    for (uint64_t now = 1000000; now < 2000000; now += 10000) app.tick(now);
    unsigned attacks = 0;
    for (const auto& event : sink.events) {
        if (event.type != MidiType::NoteOn) continue;
        EXPECT(engine.contains(0, ScaleType::Nikriz, event.data1));
        ++attacks;
    }
    EXPECT(attacks >= 3);
    app.receive(MidiEvent::noteOff(0, 62, 0, 2000000));
    app.tick(2000000);
    EXPECT(app.activeNotes().size() == 0);
    EXPECT(app.scheduler().empty());
}
