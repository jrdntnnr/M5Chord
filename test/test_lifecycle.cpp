#include "TestSupport.h"
#include "app/App.h"

using namespace midibrain;

void testLifecycle() {
    test::FakeSink sink;
    App app(sink);
    app.receive(MidiEvent::noteOn(9, 48, 100, 1));
    EXPECT(sink.events.size() == 1 && sink.events[0].channel == 9);
    sink.events.clear();
    app.apply({SemanticAction::OutputLaneNext, 0, true}, 2);
    sink.events.clear();
    app.receive(MidiEvent::noteOn(9, 48, 100, 3));
    EXPECT(sink.events.size() == 1 && sink.events[0].channel == 1);
    sink.events.clear();
    app.apply({SemanticAction::ModeChord, 0, true}, 0);
    sink.events.clear();
    app.receive(MidiEvent::noteOn(0, 60, 100, 100));
    app.tick(100);
    EXPECT(app.activeNotes().size() == 3);
    app.receive(MidiEvent::noteOff(0, 60, 0, 200));
    EXPECT(app.activeNotes().empty());
    EXPECT(app.scheduler().empty());
    app.receive(MidiEvent::noteOn(0, 60, 100, 300));
    app.tick(300);
    const std::size_t beforeEdit = sink.events.size();
    app.apply({SemanticAction::ExtensionMajor7, 1, true}, 350);
    app.tick(350);
    EXPECT(app.activeNotes().size() == 4);
    EXPECT(sink.events.size() == beforeEdit + 1);
    EXPECT(sink.events[sink.events.size() - 1].type == MidiType::NoteOn);
    EXPECT(sink.events[sink.events.size() - 1].data1 == 71);
    app.apply({SemanticAction::ExtensionMajor7, 0, false}, 360);
    app.tick(360);
    EXPECT(app.activeNotes().size() == 3);
    EXPECT((app.state().harmonic.extensions & ExtensionMajor7) == 0);
    app.apply({SemanticAction::ExtensionStackToggle, 0, true}, 370);
    EXPECT(app.state().harmonic.extension_stack);
    app.apply({SemanticAction::ExtensionMajor7, 1, true}, 380);
    app.apply({SemanticAction::ExtensionMajor7, 0, false}, 390);
    EXPECT((app.state().harmonic.extensions & ExtensionMajor7) != 0);
    app.apply({SemanticAction::ExtensionMajor7, 1, true}, 395);
    EXPECT((app.state().harmonic.extensions & ExtensionMajor7) == 0);
    app.panic(400);
    EXPECT(app.activeNotes().empty());
    EXPECT(app.scheduler().empty());

    app.receive(MidiEvent::noteOn(0, 62, 100, 500));
    app.receive(MidiEvent::noteOn(0, 62, 90, 501));
    app.tick(501);
    app.receive(MidiEvent::noteOff(0, 62, 0, 600));
    EXPECT(app.activeNotes().empty());
}
