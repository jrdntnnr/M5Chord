#include "TestSupport.h"
#include "ui/LiveKeyboard.h"
#include <bitset>

using namespace midibrain;

namespace {
class RejectSink final : public MidiSink {
public:
    bool send(const MidiEvent&) override { return false; }
};

void expectGates(const App& app, const test::FakeSink& sink, uint64_t nowUs) {
    std::array<std::bitset<128>, 16> expected{};
    for (const auto& event : sink.events) {
        if (event.type == MidiType::NoteOn && event.data2) expected[event.channel].set(event.data1);
        else if (event.type == MidiType::NoteOff || event.type == MidiType::NoteOn) expected[event.channel].reset(event.data1);
        else if (event.type == MidiType::ControlChange && (event.data1 == 120 || event.data1 == 123)) expected[event.channel].reset();
    }
    for (int channel = 0; channel < 16; ++channel) EXPECT(app.outputActivity().snapshot(channel, nowUs).gates == expected[channel]);
}
}

void testLiveKeyboard() {
    {
        MidiOutputActivity activity;
        activity.observe(MidiEvent::noteOn(0, 60, 100), 1000);
        activity.observe(MidiEvent::noteOn(1, 60, 100), 2000);
        activity.observe(MidiEvent::noteOff(0, 60), 3000);
        EXPECT(!activity.snapshot(0, 3000).gates[60]);
        EXPECT(activity.snapshot(1, 3000).gates[60]);
        EXPECT(activity.snapshot(-1, 3000).gates[60]);
        EXPECT(activity.snapshot(0, 3000).attacks[60]);
        EXPECT(activity.snapshot(0, 51000).attacks.none());
        EXPECT(activity.snapshot(0, 999).attacks.none());
        activity.observe(MidiEvent::noteOn(1, 60, 0), 4000);
        EXPECT(activity.snapshot(-1, 4000).gates.none());
        activity.observe(MidiEvent::noteOn(2, 64, 100), 5000);
        activity.observe(MidiEvent::cc(2, 123, 0), 6000);
        EXPECT(activity.snapshot(2, 6000).gates.none());
        EXPECT(activity.snapshot(2, 6000).attacks.none());
        for (unsigned i = 0; i < 80; ++i) activity.observe(MidiEvent::noteOn(3, i, 100), 7000 + i);
        EXPECT(activity.snapshot(3, 7100).latest == 79);
        EXPECT(activity.snapshot(3, 7100).attacks.count() == 64);
        activity.observe(MidiEvent::cc(3, 120, 0), 8000);
        EXPECT(activity.snapshot(3, 8000).gates.none());
        activity.observe(MidiEvent::noteOn(16, 60, 100), 9000);
        activity.observe(MidiEvent::noteOn(0, 128, 100), 9000);
        EXPECT(activity.snapshot(16, 9000).gates.none());
        activity.clear();
        EXPECT(activity.snapshot(-1, 9000).attacks.none());
        EXPECT(activity.snapshot(-1, 9000).gates.none());
    }
    {
        test::FakeSink sink;
        App app(sink);
        app.state().mode = EngineMode::Key;
        app.state().performance.mode = PerformanceMode::Arp;
        app.state().performance.bpm = 120;
        app.state().performance.gate_percent = 50;
        app.state().routing.raw_chord_enabled = true;
        app.state().routing.raw_chord_channel = 2;
        app.receive(MidiEvent::noteOn(0, 60, 100, 1000000));
        app.tick(1000000);
        LiveKeyboard keyboard;
        keyboard.update(app, 1000000);
        EXPECT(keyboard.gate(60));
        EXPECT(!keyboard.gate(64));
        EXPECT(!keyboard.gate(67));
        EXPECT(keyboard.attack(60));
        EXPECT(keyboard.latest() == 60);
        EXPECT(keyboard.base() == 60);
        EXPECT(app.outputActivity().snapshot(2, 1000000).gates.count() == 3);
        app.tick(1062500);
        keyboard.update(app, 1062500);
        EXPECT(!keyboard.gate(60));
        EXPECT(!keyboard.attack(60));
        app.tick(1125000);
        keyboard.update(app, 1125000);
        EXPECT(keyboard.gate(64));
        EXPECT(!keyboard.gate(60));
        EXPECT(keyboard.latest() == 64);
        app.tick(1250000);
        keyboard.update(app, 1250000);
        EXPECT(keyboard.gate(67));
        EXPECT(!keyboard.gate(64));
        EXPECT(keyboard.latest() == 67);
        app.receive(MidiEvent::noteOff(0, 60, 0, 1250001));
        keyboard.update(app, 1250001);
        EXPECT(!keyboard.gate(67));
        app.panic(1250002);
        keyboard.update(app, 1250002);
        EXPECT(keyboard.latest() == 128);
        EXPECT(!keyboard.attack(67));
        expectGates(app, sink, 1250002);
    }
    {
        test::FakeSink sink;
        App app(sink);
        app.state().mode = EngineMode::Key;
        app.state().performance.mode = PerformanceMode::Strum;
        app.state().performance.strum_interval_ms = 25;
        app.receive(MidiEvent::noteOn(0, 60, 100, 1000000));
        app.tick(1000000);
        EXPECT(app.outputActivity().snapshot(0, 1000000).gates.count() == 1);
        app.tick(1024999);
        EXPECT(app.outputActivity().snapshot(0, 1024999).gates.count() == 1);
        app.tick(1025000);
        EXPECT(app.outputActivity().snapshot(0, 1025000).gates.count() == 2);
        EXPECT(app.outputActivity().snapshot(0, 1025000).latest == 64);
        app.tick(1050000);
        EXPECT(app.outputActivity().snapshot(0, 1050000).gates.count() == 3);
        EXPECT(app.outputActivity().snapshot(0, 1050000).latest == 67);
        app.receive(MidiEvent::noteOff(0, 60, 0, 1050001));
        EXPECT(app.outputActivity().snapshot(0, 1050001).gates.none());
        app.tick(2000000);
        EXPECT(app.outputActivity().snapshot(0, 2000000).attacks.none());
    }
    for (auto mode : {PerformanceMode::Block, PerformanceMode::Strum, PerformanceMode::StrumTwoOctaves, PerformanceMode::Slop,
                      PerformanceMode::Arp, PerformanceMode::ArpTwoOctaves, PerformanceMode::Pattern, PerformanceMode::Harp}) {
        for (auto direction : {Direction::Up, Direction::Down, Direction::UpDown, Direction::Random}) {
            test::FakeSink sink;
            App app(sink);
            app.state().mode = EngineMode::Key;
            app.state().performance.mode = mode;
            app.state().performance.direction = direction;
            app.state().performance.bpm = 300;
            app.state().performance.gate_percent = 1;
            app.state().harmonic.extensions = 15;
            app.receive(MidiEvent::noteOn(0, 60, 100, 1000000));
            for (uint64_t now = 1000000; now <= 1350000; now += 1000) {
                app.tick(now);
                expectGates(app, sink, now);
            }
            app.receive(MidiEvent::noteOff(0, 60, 0, 1350001));
            app.tick(2000000);
            expectGates(app, sink, 2000000);
            EXPECT(app.outputActivity().snapshot(-1, 2000000).gates.none());
            EXPECT(app.outputActivity().snapshot(-1, 2000000).attacks.none());
        }
    }
    {
        test::FakeSink sink;
        App app(sink);
        app.receive(MidiEvent::noteOn(4, 60, 100, 1000000));
        app.receive(MidiEvent::noteOn(4, 60, 100, 1000001));
        app.receive(MidiEvent::noteOn(5, 60, 100, 1000002));
        app.receive(MidiEvent::noteOff(4, 60, 0, 1010000));
        LiveKeyboard keyboard;
        keyboard.update(app, 1010000);
        EXPECT(keyboard.gate(60));
        app.receive(MidiEvent::noteOff(5, 60, 0, 1010001));
        keyboard.update(app, 1010001);
        EXPECT(!keyboard.gate(60));
        app.receive(MidiEvent::noteOn(0, 127, 100, 1020000));
        keyboard.update(app, 1020000);
        EXPECT(keyboard.base() <= 127 && keyboard.base() + keyboard.span() > 127);
        EXPECT(!keyboard.gate(128));
        app.onUsbDisconnected(1020001);
        keyboard.update(app, 1020001);
        EXPECT(!keyboard.gate(127));
        EXPECT(!keyboard.attack(127));
    }
    {
        test::FakeSink sink;
        App app(sink);
        app.state().loop_bars = 1;
        app.state().performance.bpm = 120;
        app.apply({SemanticAction::LoopRecord, 1, true}, 1000000);
        app.receive(MidiEvent::noteOn(0, 60, 100, 1000000));
        app.receive(MidiEvent::noteOff(0, 60, 0, 1250000));
        app.tick(3000000);
        EXPECT(app.outputActivity().snapshot(0, 3000000).gates[60]);
        app.receive(MidiEvent::noteOn(0, 60, 100, 3010000));
        app.stopLoop(3020000);
        EXPECT(app.outputActivity().snapshot(0, 3020000).gates[60]);
        app.receive(MidiEvent::noteOff(0, 60, 0, 3030000));
        EXPECT(!app.outputActivity().snapshot(0, 3030000).gates[60]);
        expectGates(app, sink, 3030000);
    }
    {
        RejectSink sink;
        App app(sink);
        app.receive(MidiEvent::noteOn(0, 60, 100, 1000000));
        EXPECT(app.outputActivity().snapshot(-1, 1000000).gates.none());
        EXPECT(app.outputActivity().snapshot(-1, 1000000).attacks.none());
    }
    {
        test::FakeSink sink;
        App app(sink);
        app.state().mode = EngineMode::Key;
        app.state().performance.mode = PerformanceMode::Strum;
        app.receive(MidiEvent::noteOn(0, 60, 100, 1000000));
        app.tick(1000000);
        app.tick(1100000);
        EXPECT(app.outputActivity().snapshot(0, 1100000).attacks[64]);
        EXPECT(app.outputActivity().snapshot(0, 1100000).attacks[67]);
        app.apply({SemanticAction::OutputLaneNext, 1, true}, 1100001);
        EXPECT(app.outputActivity().snapshot(-1, 1100001).gates.none());
        EXPECT(app.outputActivity().snapshot(-1, 1100001).attacks.none());
    }
}
