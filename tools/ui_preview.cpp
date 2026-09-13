#include "ui/Ui.h"
#include <cstdio>
#include <filesystem>

using namespace midibrain;

class PreviewSink final : public MidiSink {
public:
    bool send(const MidiEvent&) override { return true; }
};

void dispatch(void* context, const ActionEvent& action, uint64_t nowUs) {
    static_cast<App*>(context)->apply(action, nowUs);
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::filesystem::create_directories(argv[1]);
    PreviewSink sink;
    App app(sink);
    ControllerMapper mapper;
    ControllerMapping mapping;
    mapping.source = {MidiType::ControlChange, 15, 127, 0, 127, -1};
    mapping.action = SemanticAction::ExtensionStackToggle;
    mapper.add(mapping);
    ProfileStore profiles;
    InputKeys input(app, mapper, profiles, dispatch, &app);
    UsbMidiSource usb(nullptr, nullptr, nullptr);
    BleMidiSource ble(nullptr, nullptr, nullptr);
    Ui ui;
    ui.setView(DisplayView::Chord);
    ui.begin();
    ui.showBootStatus("MIDI OUTPUT");
    char bootPath[512]{};
    std::snprintf(bootPath, sizeof(bootPath), "%s/00-boot.ppm", argv[1]);
    if (!ui.savePreview(bootPath)) return 1;
    uint64_t now = 1000000;
    bool success = true;
    const auto capture = [&](const char* name) {
        now += 2000000;
        ui.update(now, app, usb, ble, profiles, input);
        char path[512]{};
        std::snprintf(path, sizeof(path), "%s/%s.ppm", argv[1], name);
        success = ui.savePreview(path) && success;
    };
    capture("01-bypass");
    input.openFiles();
    capture("file-browser-empty");
    input.fileCatalog().add("Ambient-Zero-four-layer-demo.mid");
    input.fileCatalog().add("A-very-long-MIDI-file-name-for-the-two-line-browser-display.mid");
    capture("file-browser-long");
    input.handleAction({SemanticAction::MenuDown, 1, true}, now);
    capture("file-browser-selected");
    input.selectMidiPlayback();
    capture("file-playback-empty");
    input.handleAction({SemanticAction::OptionsToggle, 1, true}, now);
    app.state().mode = EngineMode::Key;
    capture("02-key-idle");
    app.apply({SemanticAction::ChordMin, 1, true, 119}, now + 1);
    capture("03-key-override");
    app.apply({SemanticAction::ChordMin, 0, false, 119}, now + 1);
    app.apply({SemanticAction::ExtensionMinor7, 1, true, 115}, now + 2);
    app.receive(MidiEvent::noteOn(0, 60, 100, now + 3));
    app.tick(now + 3);
    capture("04-key-minor-seven");
    app.apply({SemanticAction::ExtensionMajor7, 1, true, 100}, now + 1);
    capture("05-both-sevenths");
    ui.nextView();
    capture("06-notes");
    ui.nextView();
    capture("07-keyboard");
    ui.nextView();
    capture("08-diagnostics");
    ui.nextView();
    app.panic(now + 1);
    app.state().mode = EngineMode::Chord;
    app.apply({SemanticAction::ChordMin, 1, true, 119}, now + 2);
    app.apply({SemanticAction::ChordMin, 0, false, 119}, now + 3);
    capture("09-chord-latched");
    app.state().harmonic.scale = ScaleType::MajorPentatonic;
    app.state().harmonic.key_root = 10;
    app.state().harmonic.harmonic_quantize = true;
    capture("10-long-scale");
    app.state().harmonic.extension_stack = true;
    app.state().harmonic.extensions = 15;
    app.state().performance.mode = PerformanceMode::ArpTwoOctaves;
    app.state().performance.bpm = 300;
    app.state().harmonic.voicing_step = -8;
    app.receive(MidiEvent::noteOn(0, 60, 100, now + 1));
    capture("11-stacked-arp");
    for (auto style : {PlayStyle::Simple, PlayStyle::Advanced, PlayStyle::Free}) {
        app.panic(now + 1);
        app.state().harmonic.play_style = style;
        char name[32]{};
        std::snprintf(name, sizeof(name), "12-style-%u", static_cast<unsigned>(style));
        capture(name);
    }
    input.handleAction({SemanticAction::OptionsToggle, 1, true}, now + 1);
    capture("13-options-mode");
    for (unsigned i = 0; i < 5; ++i) input.handleAction({SemanticAction::MenuDown, 1, true}, now + 2 + i);
    capture("14-options-scale");
    app.state().harmonic.harmonic_quantize = false;
    capture("15-options-scale-off");
    for (std::size_t row = 0; row < input.menuCount(); ++row) {
        char name[32]{};
        std::snprintf(name, sizeof(name), "menu-%02u", static_cast<unsigned>(input.menuIndex()));
        capture(name);
        input.handleAction({SemanticAction::MenuDown, 1, true}, now + 1);
    }
    input.handleAction({SemanticAction::MenuBack, 1, true}, now + 1);
    input.handleAction({SemanticAction::PadSetup, 1, true}, now + 2);
    capture("16-pad-setup");
    input.handleAction({SemanticAction::MenuBack, 1, true}, now + 1);
    app.apply({SemanticAction::KeyLearn, 1, true}, now + 2);
    capture("17-key-learn");
    app.panic(now + 1);
    app.state().harmonic.harmonic_quantize = true;
    ui.showOverlay("Harmonic quantize: ON", now + 3);
    ui.update(now + 4, app, usb, ble, profiles, input);
    char path[512]{};
    std::snprintf(path, sizeof(path), "%s/18-toast.ppm", argv[1]);
    success = ui.savePreview(path) && success;
    input.handleAction({SemanticAction::MappingEdit, 1, true}, now + 5);
    for (unsigned field = 0; field < 5; ++field) {
        char name[32]{};
        std::snprintf(name, sizeof(name), "19-mapping-%u", field);
        capture(name);
        input.handleAction({SemanticAction::MenuDown, 1, true}, now + 1);
    }
    input.handleAction({SemanticAction::MenuBack, 1, true}, now + 1);
    app.panic(now + 2);
    app.state().mode = EngineMode::Key;
    app.state().harmonic.key_root = 0;
    app.state().harmonic.scale = ScaleType::Major;
    app.state().harmonic.play_style = PlayStyle::Latched;
    app.state().harmonic.harmonic_quantize = false;
    app.state().harmonic.extensions = 0;
    app.state().harmonic.voicing_step = 0;
    app.state().harmonic.extension_stack = false;
    app.state().performance.mode = PerformanceMode::Block;
    app.receive(MidiEvent::noteOn(0, 60, 100, now + 3));
    app.tick(now + 3);
    capture("20-large-major");
    app.panic(now + 1);
    app.state().mode = EngineMode::Chord;
    app.state().harmonic.quality = ChordQuality::Suspended;
    app.state().harmonic.extensions = 15;
    app.receive(MidiEvent::noteOn(0, 61, 100, now + 2));
    app.tick(now + 2);
    capture("21-long-chord");
    ui.nextView();
    ui.nextView();
    constexpr const char* names[]{"block", "strum", "strum2", "slop", "arp", "arp2", "pattern", "harp"};
    for (unsigned mode = 0; mode < 8; ++mode) {
        now += 2000000;
        app.panic(now);
        app.state().mode = EngineMode::Key;
        app.state().harmonic.extensions = 0;
        app.state().performance.mode = static_cast<PerformanceMode>(mode);
        app.state().performance.bpm = 120;
        app.state().performance.gate_percent = 50;
        app.state().performance.strum_interval_ms = 50;
        app.receive(MidiEvent::noteOn(0, 60, 100, now));
        for (unsigned ms = 0; ms < 1000; ++ms) {
            if (ms == 750) app.receive(MidiEvent::noteOff(0, 60, 0, now + ms * 1000));
            app.tick(now + ms * 1000);
            if (ms % 20) continue;
            ui.update(now + ms * 1000, app, usb, ble, profiles, input);
            std::snprintf(path, sizeof(path), "%s/seq-%s-%03u.ppm", argv[1], names[mode], ms / 20);
            success = ui.savePreview(path) && success;
        }
    }
    input.handleAction({SemanticAction::HelpToggle, 1, true}, now + 1);
    for (std::size_t page = 0; page < HelpPages; ++page) {
        char name[32]{};
        std::snprintf(name, sizeof(name), "help-%02u", static_cast<unsigned>(page));
        capture(name);
        input.handleAction({SemanticAction::MenuIncrease, 1, true}, now + 1);
    }
    input.handleAction({SemanticAction::HelpToggle, 1, true}, now + 1);
    input.selectMidiPlayback();
    capture("player-empty");
    app.midiFile().setName("ghaetta.mid");
    input.setFileLoading(true, 42);
    capture("player-loading");
    input.setFileLoading(false, 100);
    app.midiFile().file().error = "SD TOO SLOW: STOPPED";
    app.midiFile().stop();
    capture("player-error");
    app.midiFile().file().error = nullptr;
    app.midiFile().file().channels = 31;
    app.midiFile().file().duration_us = 368562748;
    app.midiFile().stop(); input.selectMidiPlayback();
    capture("player-ready");
    app.midiFile().play(now, 4);
    now += 60000000;
    capture("player-playing");
    app.midiFile().setName("A-very-long-MIDI-song-name-with-several-parts-and-versions.mid");
    capture("player-long-name");
    input.handleAction({SemanticAction::OptionsToggle, 1, true}, now + 1);
    app.panic(now + 2);
    app.state().mode = EngineMode::Chord;
    app.state().harmonic.quality = ChordQuality::Major;
    app.state().harmonic.extensions = ExtensionMajor7;
    app.state().harmonic.extension_stack = true;
    app.state().harmonic.play_style = PlayStyle::Free;
    app.state().harmonic.scale = ScaleType::Major;
    app.state().harmonic.key_root = 0;
    app.state().harmonic.voicing_step = 0;
    app.state().harmonic.harmonic_quantize = false;
    app.state().performance.mode = PerformanceMode::Block;
    app.state().performance.bpm = 100;
    app.state().routing.performance_channel = 0;
    app.state().bass.mode = BassMode::Off;
    app.state().routing.raw_chord_enabled = false;
    app.apply({SemanticAction::ChordMaj, 1, true}, now + 3);
    app.receive(MidiEvent::noteOn(0, 60, 100, now + 3));
    app.tick(now + 3);
    ui.setView(DisplayView::Keyboard);
    capture("M5Chord-keyboard");
    return success ? 0 : 1;
}
