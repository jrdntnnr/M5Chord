#include "TestSupport.h"
#include "app/App.h"
#include "hardware/InputKeys.h"
#include "ui/HarmonyDisplay.h"
#include <cstring>
#include <initializer_list>

using namespace midibrain;

namespace {
void expectNotes(const NoteList& notes, std::initializer_list<uint8_t> expected) {
    EXPECT(notes.size() == expected.size());
    unsigned index = 0;
    for (const auto note : expected) EXPECT(notes[index++] == note);
}
}

void testKeyExtensions() {
    ScaleEngine scales;
    expectNotes(scales.buildDiatonic(60, 0, ScaleType::Major, Extension6), {60, 64, 67, 69});
    expectNotes(scales.buildDiatonic(60, 0, ScaleType::Major, ExtensionMinor7), {60, 64, 67, 70});
    expectNotes(scales.buildDiatonic(60, 0, ScaleType::Major, ExtensionMajor7), {60, 64, 67, 71});
    expectNotes(scales.buildDiatonic(60, 0, ScaleType::Major, Extension9), {60, 64, 67, 74});
    expectNotes(scales.buildDiatonic(62, 0, ScaleType::Major, ExtensionMajor7), {62, 65, 69, 73});
    expectNotes(scales.buildDiatonic(60, 0, ScaleType::NaturalMinor, Extension6), {60, 63, 67, 69});
    expectNotes(scales.buildDiatonic(60, 0, ScaleType::Major, 15), {60, 64, 67, 69, 70, 71, 74});
    expectNotes(scales.buildDiatonic(63, 0, ScaleType::Major, Extension6), {62, 65, 69, 71});
    for (uint8_t scale = 0; scale < ScaleCount; ++scale) {
        for (unsigned root = 0; root < 128; ++root) {
            for (uint8_t extensions = 0; extensions < 16; ++extensions) {
                const auto notes = scales.buildDiatonic(root, 0, static_cast<ScaleType>(scale), extensions);
                EXPECT(!notes.empty());
                for (std::size_t i = 0; i < notes.size(); ++i) {
                    EXPECT(notes[i] <= 127);
                    if (i) EXPECT(notes[i] > notes[i - 1]);
                }
            }
        }
    }
    test::FakeSink sink;
    App app(sink);
    app.state().mode = EngineMode::Key;
    app.receive(MidiEvent::noteOn(0, 60, 100, 100));
    app.tick(100);
    const auto attacks = sink.events.size();
    app.apply({SemanticAction::Extension6, 1, true, 97}, 200);
    app.tick(200);
    expectNotes(app.lastChord(), {60, 64, 67, 69});
    EXPECT(sink.events.size() == attacks + 1);
    EXPECT(sink.events[sink.events.size() - 1].data1 == 69);
    app.apply({SemanticAction::ExtensionMinor7, 1, true, 115}, 300);
    app.apply({SemanticAction::ExtensionMajor7, 1, true, 100}, 301);
    app.tick(301);
    expectNotes(app.lastChord(), {60, 64, 67, 69, 70, 71});
    app.apply({SemanticAction::ExtensionMinor7, 0, false, 115}, 400);
    app.tick(400);
    expectNotes(app.lastChord(), {60, 64, 67, 69, 71});
    EXPECT(sink.events[sink.events.size() - 1].type == MidiType::NoteOff);
    EXPECT(sink.events[sink.events.size() - 1].data1 == 70);
    app.receive(MidiEvent::noteOn(0, 67, 100, 500));
    app.tick(500);
    app.receive(MidiEvent::noteOff(0, 60, 0, 600));
    app.receive(MidiEvent::noteOff(0, 67, 0, 601));
    app.tick(601);
    EXPECT(app.activeNotes().empty());
    EXPECT(app.scheduler().empty());
    app.apply({SemanticAction::ExtensionStackToggle, 1, true}, 700);
    app.apply({SemanticAction::Extension9, 1, true, 102}, 701);
    app.apply({SemanticAction::Extension9, 0, false, 102}, 702);
    app.receive(MidiEvent::noteOn(0, 60, 100, 800));
    app.tick(800);
    expectNotes(app.lastChord(), {60, 64, 67, 74});
    app.apply({SemanticAction::Extension9, 1, true, 102}, 900);
    app.tick(900);
    expectNotes(app.lastChord(), {60, 64, 67});
    app.panic(1000);
    uint64_t start = 2000000;
    for (auto mode : {PerformanceMode::Arp, PerformanceMode::Strum, PerformanceMode::Pattern}) {
        app.state().performance.mode = mode;
        app.state().harmonic.extensions = 15;
        app.receive(MidiEvent::noteOn(0, 60, 100, start));
        for (uint64_t now = start; now < start + 200000; now += 10000) app.tick(now);
        app.apply({SemanticAction::ExtensionMajor7, 1, true, 100}, start + 200000);
        app.receive(MidiEvent::noteOff(0, 60, 0, start + 300000));
        app.tick(start + 300000);
        start += 1000000;
        EXPECT(app.activeNotes().empty());
        EXPECT(app.scheduler().empty());
    }
    HarmonyDisplay display;
    app.panic(3000000);
    app.state().harmonic.harmonic_quantize = false;
    app.state().mode = EngineMode::Bypass;
    display.update(app, 3000001);
    EXPECT(display.quality(2) == PadDisplayState::Disabled);
    EXPECT(display.extension(0) == PadDisplayState::Disabled);
    EXPECT(!display.qualitySelected(2));
    app.state().mode = EngineMode::Chord;
    display.update(app, 3000002);
    EXPECT(std::strcmp(display.scaleStatus(), "SNAP OFF") == 0);
    EXPECT(display.qualitySelected(2));
    app.state().mode = EngineMode::Key;
    display.update(app, 3000003);
    EXPECT(std::strcmp(display.scaleStatus(), "AUTO SCALE") == 0);
    EXPECT(!display.qualitySelected(2));
    app.apply({SemanticAction::ChordMin, 1, true, 119}, 3000004);
    app.apply({SemanticAction::ChordMaj, 1, true, 101}, 3000005);
    display.update(app, 3000006);
    EXPECT(std::strcmp(display.scaleStatus(), "OVERRIDE") == 0);
    EXPECT(!display.keyActive());
    EXPECT(display.quality(1) == PadDisplayState::Held);
    EXPECT(!display.qualitySelected(1));
    EXPECT(display.qualitySelected(2));
    app.apply({SemanticAction::HarmonicQuantizeToggle, 1, true}, 3000007);
    display.update(app, 3000008);
    EXPECT(display.keyActive());
    EXPECT(std::strcmp(display.scaleStatus(), "SNAP ON") == 0);
    ControllerMapper mapper;
    ProfileStore profiles;
    InputKeys input(app, mapper, profiles, nullptr, nullptr);
    input.handleAction({SemanticAction::OptionsToggle, 1, true}, 4000000);
    EXPECT(input.menuIndex() == 0);
    EXPECT(std::strcmp(input.menuNeighbor(1), "Play style") == 0);
    input.handleAction({SemanticAction::MenuUp, 1, true}, 4000001);
    EXPECT(input.menuIndex() == input.menuCount() - 1);
    EXPECT(std::strcmp(input.menuHelp(), "Enter runs this command") == 0);
    char formatted[32]{};
    formatParameter(30, app.state(), formatted, sizeof(formatted));
    EXPECT(std::strcmp(formatted, "ON") == 0);
}
