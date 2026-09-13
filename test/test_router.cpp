#include "TestSupport.h"
#include "app/App.h"

using namespace midibrain;

void testRouter() {
    test::FakeSink sink;
    App app(sink);
    app.receive(MidiEvent::cc(4, 1, 90, 0));
    EXPECT(sink.events.size() == 1 && sink.events[0].channel == 4);
    sink.events.clear();
    app.apply({SemanticAction::ModeChord, 0, true}, 1);
    sink.events.clear();
    app.receive(MidiEvent::cc(4, 1, 90, 2));
    EXPECT(sink.events.size() == 1 && sink.events[0].channel == 0);
}
