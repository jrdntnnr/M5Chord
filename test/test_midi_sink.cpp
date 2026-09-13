#include "TestSupport.h"
#include "transport/DinMidiSink.h"

using namespace midibrain;

void testMidiSink() {
    DinMidiSink sink;
    EXPECT(sink.send(MidiEvent::noteOn(0, 60, 100)));
    EXPECT(sink.send(MidiEvent::noteOff(0, 60)));
    EXPECT(sink.send(MidiEvent::cc(0, 1, 90)));
    EXPECT(sink.queued() == 3);
    sink.discardPending();
    EXPECT(sink.queued() == 1);
}
