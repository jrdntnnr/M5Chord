#include "TestSupport.h"
#include "scheduler/MidiScheduler.h"

using namespace midibrain;

void testScheduler() {
    MidiScheduler scheduler;
    const VoiceId first{0, 60, 1};
    const VoiceId second{0, 62, 2};
    scheduler.schedule(100, first, StreamId::Performance, MidiEvent::noteOn(0, 60, 100));
    scheduler.schedule(100, second, StreamId::Performance, MidiEvent::noteOn(0, 62, 100));
    scheduler.schedule(50, first, StreamId::Performance, MidiEvent::noteOn(0, 64, 100));
    ScheduledMidiEvent event;
    EXPECT(!scheduler.popDue(49, event));
    EXPECT(scheduler.popDue(50, event) && event.event.data1 == 64);
    EXPECT(scheduler.popDue(100, event) && event.event.data1 == 60);
    EXPECT(scheduler.cancel(second) == 1);
    EXPECT(scheduler.empty());
    EXPECT(scheduler.stats().high_water == 3);

    for (std::size_t i = 0; i < MidiScheduler::Capacity; ++i) {
        EXPECT(scheduler.schedule(i, first, StreamId::Performance, MidiEvent::noteOn(0, static_cast<uint8_t>(i % 128), 100)));
    }
    EXPECT(scheduler.schedule(0, first, StreamId::Performance, MidiEvent::noteOff(0, 60)));
    EXPECT(scheduler.size() == MidiScheduler::Capacity);
}
