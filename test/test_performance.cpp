#include "TestSupport.h"
#include "engine/PerformanceEngine.h"

using namespace midibrain;

void testPerformance() {
    NoteList notes;
    notes.push_back(60);
    notes.push_back(64);
    notes.push_back(67);
    PerformanceEngine engine(1);
    PerformanceConfig config;
    config.mode = PerformanceMode::Strum;
    config.strum_interval_ms = 25;
    MidiScheduler scheduler;
    const VoiceId owner{0, 60, 1};
    EXPECT(engine.trigger(notes, 0, 100, 1000, owner, config, scheduler));
    ScheduledMidiEvent event;
    EXPECT(scheduler.popDue(1000, event) && event.event.data1 == 60);
    EXPECT(scheduler.popDue(26000, event) && event.event.data1 == 64);
    EXPECT(scheduler.popDue(51000, event) && event.event.data1 == 67);
    EXPECT(engine.stepDurationUs(120, Division::Sixteenth) == 125000);
    scheduler.clear();
    for (std::size_t i=0;i<MidiScheduler::Capacity-1;++i) EXPECT(scheduler.schedule(10000,owner,StreamId::Performance,MidiEvent::noteOff(0,60)));
    config.mode = PerformanceMode::Arp;
    EXPECT(!engine.trigger(notes,0,100,1000,owner,config,scheduler));
    EXPECT(scheduler.size() == MidiScheduler::Capacity-1);
}
