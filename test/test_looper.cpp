#include "TestSupport.h"
#include "app/App.h"
#include "engine/MidiLooper.h"
#include "storage/LoopCodec.h"

using namespace midibrain;

void testLooper() {
    {
        MidiLooper loop;
        loop.record(1000000,120,1,0);
        loop.capture(MidiEvent::noteOn(0,60,100),1000000);
        loop.stop(1250000);
        EXPECT(loop.length() == 384);
        EXPECT(loop.entries()[0].duration == 48);
        loop.play(2000000);
        loop.overdub(2000000);
        loop.capture(MidiEvent::noteOn(0,64,100),2100000);
        loop.overdub(2200000);
        EXPECT(loop.entries()[1].duration == 19);
    }
    {
        MidiLooper loop;
        MidiScheduler scheduler;
        loop.record(1000000,120,1,0);
        loop.capture(MidiEvent::noteOn(2,60,100),1000000);
        loop.capture(MidiEvent::noteOff(2,60),1250000);
        EXPECT(loop.entries().size() == 1);
        EXPECT(loop.entries()[0].duration == 48);
        loop.tick(3000000,scheduler);
        EXPECT(loop.mode() == LoopMode::Playing);
        EXPECT(loop.length() == 384);
        EXPECT(scheduler.size() == 2);
        loop.overdub(3050000);
        EXPECT(loop.mode() == LoopMode::Overdub);
        loop.capture(MidiEvent::noteOn(2,64,90),3100000);
        loop.capture(MidiEvent::noteOff(2,64),3200000);
        EXPECT(loop.entries().size() == 2);
        loop.undo(3300000);
        EXPECT(loop.mode() == LoopMode::Stopped);
        EXPECT(loop.entries().size() == 1);
        JsonDocument document;
        LoopCodec::encode(loop,document);
        MidiLooper restored;
        EXPECT(LoopCodec::decode(document,restored));
        EXPECT(restored.length() == 384);
        EXPECT(restored.entries()[0].duration == 48);
        document["events"][0][3] = 16;
        EXPECT(!LoopCodec::decode(document,restored));
        EXPECT(restored.entries().size() == 1);
        LoopCodec::encode(loop,document);
        document["events"][0][5] = 99;
        EXPECT(!LoopCodec::decode(document,restored));
        loop.clear();
        EXPECT(loop.entries().empty());
    }
    {
        test::FakeSink sink;
        App app(sink);
        app.state().loop_bars = 1;
        app.state().performance.bpm = 120;
        app.apply({SemanticAction::LoopRecord,1,true},1000000);
        app.receive(MidiEvent::noteOn(0,60,100,1000000));
        app.receive(MidiEvent::noteOff(0,60,0,1250000));
        app.tick(3000000);
        EXPECT(app.looper().entries().size() == 1);
        EXPECT(app.activeNotes().size() == 1);
        app.receive(MidiEvent::noteOn(0,60,100,3010000));
        EXPECT(app.activeNotes().size() == 2);
        app.tick(3250000);
        EXPECT(app.activeNotes().size() == 1);
        EXPECT(app.looper().entries().size() == 1);
        app.receive(MidiEvent::noteOff(0,60,0,3300000));
        EXPECT(app.activeNotes().size() == 0);
        app.tick(5000000);
        EXPECT(app.activeNotes().size() == 1);
        app.apply({SemanticAction::LoopStop,1,true},5010000);
        EXPECT(app.activeNotes().size() == 0);
        EXPECT(app.scheduler().empty());
        app.tick(7000000);
        EXPECT(app.activeNotes().size() == 0);
        app.apply({SemanticAction::LoopPlay,1,true},8000000);
        app.tick(8000000);
        EXPECT(app.activeNotes().size() == 1);
        app.panic(8000001);
        EXPECT(app.looper().mode() == LoopMode::Stopped);
        EXPECT(app.activeNotes().size() == 0);
    }
    {
        MidiLooper loop;
        MidiScheduler scheduler;
        loop.record(1000000,120,0,24);
        loop.capture(MidiEvent::noteOn(0,60,100),1070000);
        loop.capture(MidiEvent::noteOff(0,60),1100000);
        EXPECT(loop.entries()[0].tick == 24);
        EXPECT(loop.entries()[0].duration > 0);
        loop.play(1500000);
        EXPECT(loop.length() == 96);
        loop.tick(1700000,scheduler);
        EXPECT(scheduler.empty());
        FixedList<LoopEntry,MidiLooper::Capacity> invalid;
        invalid.push_back({0,1,MidiType::NoteOn,16,60,100,0,0});
        EXPECT(!loop.replace(invalid,96,120));
        EXPECT(loop.entries().size() == 1);
        loop.record(2000000,120,1,0);
        for (unsigned i=0;i<MidiLooper::Capacity + 10;++i) loop.capture(MidiEvent::cc(0,1,i%128),2000000+i);
        EXPECT(loop.entries().size() == MidiLooper::Capacity);
        EXPECT(loop.dropped() == 10);
        loop.stop(2100000);
        EXPECT(loop.mode() == LoopMode::Stopped);
    }
}
