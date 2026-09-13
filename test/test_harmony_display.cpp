#include "TestSupport.h"
#include "controller/ControllerMapper.h"
#include "ui/HarmonyDisplay.h"

#include <cstring>

using namespace midibrain;

void testHarmonyDisplay() {
    test::FakeSink sink;
    App app(sink);
    HarmonyDisplay display;
    app.state().mode = EngineMode::Key;
    display.update(app,1000000);
    EXPECT(std::strcmp(display.keyLabel(),"KEY: C Major") == 0);
    EXPECT(std::strcmp(display.chordLabel(),"READY") == 0);
    EXPECT(!display.chordVisible());
    EXPECT(display.notes().empty());
    EXPECT(display.quality(2) == PadDisplayState::Off);

    app.apply({SemanticAction::ChordMin,1,true,119},1100000);
    app.apply({SemanticAction::Extension9,1,true,102},1100000);
    display.update(app,1100000);
    EXPECT(display.quality(1) == PadDisplayState::Held);
    EXPECT(display.extension(3) == PadDisplayState::Held);
    EXPECT(std::strcmp(display.chordLabel(),"READY") == 0);
    EXPECT(sink.events.empty());
    app.apply({SemanticAction::Extension9,0,false,102},1200000);
    app.apply({SemanticAction::ChordMin,0,false,119},1200000);
    display.update(app,1200000);
    EXPECT(display.extension(3) == PadDisplayState::Off);
    EXPECT(display.quality(1) == PadDisplayState::Off);

    app.state().mode = EngineMode::Chord;
    display.update(app,1300000);
    EXPECT(display.quality(1) == PadDisplayState::Selected);
    EXPECT(std::strcmp(display.keyLabel(),"KEY: C Major (OFF)") == 0);
    app.receive(MidiEvent::noteOn(0,60,100,2000000));
    app.tick(2000000);
    display.update(app,2000000);
    EXPECT(std::strcmp(display.chordLabel(),"Cm") == 0);
    EXPECT(display.brightness() == 255);
    display.update(app,10000000);
    EXPECT(display.chordVisible());
    EXPECT(display.brightness() == 255);
    app.receive(MidiEvent::noteOff(0,60,0,11000000));
    display.update(app,11000000);
    display.update(app,11950000);
    EXPECT(display.chordVisible());
    EXPECT(display.brightness() > 0 && display.brightness() < 255);
    app.apply({SemanticAction::Extension6,1,true,97},12000000);
    display.update(app,12200000);
    EXPECT(!display.chordVisible());
    EXPECT(display.notes().empty());
    EXPECT(display.extension(0) == PadDisplayState::Held);
    EXPECT(std::strcmp(display.chordLabel(),"READY") == 0);

    app.apply({SemanticAction::ExtensionStackToggle,1,true},13000000);
    app.apply({SemanticAction::ExtensionMajor7,1,true,100},13000001);
    app.apply({SemanticAction::ExtensionMajor7,0,false,100},13000002);
    display.update(app,13000003);
    EXPECT(display.extension(0) == PadDisplayState::Off);
    EXPECT(display.extension(2) == PadDisplayState::Selected);
    EXPECT(!display.chordVisible());
    app.apply({SemanticAction::ExtensionMajor7,1,true,100},13000004);
    display.update(app,13000005);
    EXPECT(display.extension(2) == PadDisplayState::Off);

    app.apply({SemanticAction::ExtensionStackToggle,1,true},14000000);
    ControllerMapper mapper;
    ControllerMapping mapping;
    mapping.source = {MidiType::NoteOn,9,36,0,127,0};
    mapping.action = SemanticAction::Extension9;
    mapping.trigger = MappingTrigger::PressRelease;
    mapping.consume = true;
    mapper.add(mapping);
    auto mapped = mapper.map(MidiEvent::noteOn(9,36,100,14000001));
    for (const auto& action : mapped.actions) app.apply(action,14000001);
    app.apply({SemanticAction::Extension9,1,true,102},14000002);
    mapped = mapper.map(MidiEvent::noteOff(9,36,0,14000003));
    for (const auto& action : mapped.actions) app.apply(action,14000003);
    display.update(app,14000004);
    EXPECT(display.extension(3) == PadDisplayState::Held);
    app.apply({SemanticAction::Extension9,0,false,102},14000005);
    display.update(app,14000006);
    EXPECT(display.extension(3) == PadDisplayState::Off);

    app.receive(MidiEvent::noteOn(0,64,100,15000000));
    app.receive(MidiEvent::noteOff(0,64,0,15000001));
    display.update(app,15000002);
    EXPECT(display.chordVisible());
    app.panic(15000003);
    display.update(app,15000004);
    EXPECT(!display.chordVisible());
    app.receive(MidiEvent::noteOff(0,64,0,15000005));
    display.update(app,15000006);
    EXPECT(!display.chordVisible());

    FixedList<LoopEntry,MidiLooper::Capacity> entries;
    entries.push_back({0,48,MidiType::NoteOn,0,60,100,0,0});
    EXPECT(app.looper().replace(entries,96,120));
    app.looper().play(16000000);
    display.update(app,16000000);
    EXPECT(std::strcmp(display.chordLabel(),"LOOP") == 0);
    EXPECT(!display.chordVisible());
    app.panic(16000001);

    app.state().mode = EngineMode::Chord;
    app.state().performance.mode = PerformanceMode::Arp;
    app.state().routing.performance_enabled = false;
    app.receive(MidiEvent::noteOn(0,60,100,17000000));
    display.update(app,17000000);
    display.update(app,27000000);
    EXPECT(app.hasHeldRoots());
    EXPECT(display.chordVisible());
}
