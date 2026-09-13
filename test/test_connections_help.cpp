#include "TestSupport.h"
#include "hardware/InputKeys.h"
#include "midi/DinMidiDecoder.h"
#include "transport/InputSelection.h"
#include "transport/BleMidiDiscovery.h"
#include "transport/BleTraceConsole.h"
#include "storage/StateCodec.h"
#include <cstring>

using namespace midibrain;

namespace {
void collect(void* context, const MidiEvent& event) { static_cast<FixedList<MidiEvent, 64>*>(context)->push_back(event); }
void action(void* context, const ActionEvent& event, uint64_t nowUs) { static_cast<App*>(context)->apply(event, nowUs); }
}

void testConnectionsHelp() {
    DinMidiDecoder decoder;
    FixedList<MidiEvent, 64> events;
    const uint8_t bytes[]{0x91, 60, 0xf8, 100, 61, 80, 60, 0, 0xc2, 5, 6, 0xf2, 1, 2, 7, 0xf0, 1, 2, 0xfa, 3, 0xf7, 64, 65, 0xe0, 0, 64};
    for (auto byte : bytes) decoder.feed(byte, 100, collect, &events);
    EXPECT(events.size() == 9);
    EXPECT(events[0].type == MidiType::Clock);
    EXPECT(events[1].type == MidiType::NoteOn && events[1].channel == 1 && events[1].data1 == 60);
    EXPECT(events[2].data1 == 61);
    EXPECT(events[3].type == MidiType::NoteOff && events[3].data1 == 60);
    EXPECT(events[4].type == MidiType::ProgramChange && events[5].data1 == 6);
    EXPECT(events[6].type == MidiType::SongPosition && events[6].value14 == 257);
    EXPECT(events[7].type == MidiType::Start);
    EXPECT(events[8].type == MidiType::PitchBend && events[8].value14 == 8192);
    EXPECT(!decoder.takeRecovery(5000000));
    decoder.feed(0xfe, 5000000, collect, &events);
    EXPECT(!decoder.takeRecovery(5300000));
    EXPECT(decoder.takeRecovery(5300001));
    EXPECT(!decoder.takeRecovery(9999999));
    decoder.feed(0xff, 10000000, collect, &events);
    EXPECT(decoder.takeRecovery(10000000));
    decoder.feed(0x90, 1, collect, &events);
    decoder.feed(60, 2, collect, &events);
    decoder.reset();
    decoder.feed(100, 3, collect, &events);
    EXPECT(events.size() == 9);

    InputSelection selection;
    EXPECT(!selection.accept(InputPort::Din, {MidiType::Clock}));
    EXPECT(selection.accept(InputPort::Ble, MidiEvent::noteOn(0, 60, 100)));
    EXPECT(!selection.accept(InputPort::Din, MidiEvent::noteOff(0, 60)));
    EXPECT(!selection.disconnected(InputPort::Usb));
    EXPECT(selection.active() == InputPort::Ble);
    EXPECT(selection.disconnected(InputPort::Ble));
    EXPECT(selection.accept(InputPort::Din, MidiEvent::noteOn(0, 60, 100)));
    selection.configure(InputPort::Usb);
    EXPECT(!selection.accept(InputPort::Ble, MidiEvent::noteOn(0, 60, 100)));
    EXPECT(selection.accept(InputPort::Usb, MidiEvent::noteOff(0, 60)));
    EXPECT(selection.disconnected(InputPort::Usb));
    EXPECT(selection.active() == InputPort::Usb);

    EXPECT(bleMidiCandidate(true, "Unrelated Brand"));
    EXPECT(bleMidiCandidate(true, ""));
    EXPECT(bleMidiCandidate(false, "Piano midi adapter"));
    EXPECT(bleMidiCandidate(false, "SMK-37 Pro_BLE"));
    EXPECT(!bleMidiCandidate(false, "SMK-37 Audio"));
    EXPECT(!bleMidiCandidate(false, "Headphones"));
    EXPECT(!bleMidiCandidate(false, nullptr));

    BleMidiTrace trace;
    BleTraceConsole console;
    char output[256]{};
    EXPECT(!console.next(trace, output, sizeof(output)));
    BleTraceRecord record;
    record.event = BleTraceEvent::Notification;
    record.length = 200;
    record.data[0] = 0x90;
    for (unsigned i = 0; i < 140; ++i) { record.at_us = i; trace.push(record); }
    EXPECT(trace.size() == 128 && trace.at(0).at_us == 12);
    EXPECT(console.next(trace, output, sizeof(output)) && std::strstr(output, "TRACE_GAP"));
    EXPECT(console.next(trace, output, sizeof(output)) && std::strstr(output, "data=90"));
    console.rewind(trace);
    EXPECT(console.next(trace, output, sizeof(output)) && std::strstr(output, "us=12"));

    for (const auto& binding : keyBindings) {
        bool found = false;
        for (const auto& shortcut : shortcuts) found |= shortcut.action == binding.action;
        EXPECT(found);
    }
    EXPECT(keyBindingsUnique());
    test::FakeSink sink;
    App app(sink);
    ControllerMapper mapper;
    ProfileStore profiles;
    InputKeys input(app, mapper, profiles, action, &app);
    EXPECT(input.handleAction({SemanticAction::HelpToggle, 1, true}, 1));
    EXPECT(input.helpOpen() && input.menuOpen());
    for (std::size_t i = 0; i < HelpPages; ++i) {
        EXPECT(input.helpPage() == i);
        input.handleAction({SemanticAction::MenuIncrease, 1, true}, 2);
    }
    EXPECT(input.helpPage() == 0);
    input.handleAction({SemanticAction::MenuDecrease, 1, true}, 3);
    EXPECT(input.helpPage() == HelpPages - 1);
    input.handleAction({SemanticAction::HelpToggle, 1, true}, 4);
    EXPECT(!input.helpOpen() && !input.menuOpen());
    input.handleAction({SemanticAction::OptionsToggle, 1, true}, 5);
    input.handleAction({SemanticAction::HelpToggle, 1, true}, 6);
    input.handleAction({SemanticAction::HelpToggle, 1, true}, 7);
    EXPECT(input.menuOpen() && !input.helpOpen());

    EXPECT(app.state().lane_count == 4);
    for (int count = 1; count <= 16; ++count) {
        app.apply({SemanticAction::LaneCountSet, static_cast<int16_t>(count), true}, 10);
        app.state().routing.performance_channel = 0;
        for (int i = 0; i < count; ++i) { sink.events.clear(); app.apply({SemanticAction::OutputLaneNext, 1, true}, 11); }
        EXPECT(app.state().routing.performance_channel == 0);
    }
    app.apply({SemanticAction::LaneCountSet, 0, true}, 12);
    EXPECT(app.state().lane_count == 1);
    app.apply({SemanticAction::LaneCountSet, 99, true}, 13);
    EXPECT(app.state().lane_count == 16);
    app.state().mode = EngineMode::Chord;
    sink.events.clear();
    app.receive(MidiEvent::noteOn(0, 60, 100, 20));
    app.tick(20);
    EXPECT(app.activeNotes().size() > 0);
    app.apply({SemanticAction::OutputLaneNext, 1, true}, 21);
    EXPECT(app.activeNotes().size() == 0 && app.scheduler().size() == 0);
    app.state().input_port = 3;
    JsonDocument document;
    StateCodec::encode(app.state(), document);
    AppState restored;
    EXPECT(StateCodec::decode(document, restored));
    EXPECT(restored.input_port == 3 && restored.lane_count == 16);
    document["lane_count"] = 0;
    EXPECT(!StateCodec::decode(document, restored));
    StateCodec::encode(app.state(), document);
    uint32_t legacy = StateCodec::fingerprint(app.state());
    for (uint32_t value : {static_cast<uint32_t>(app.state().lane_count), static_cast<uint32_t>(app.state().input_port)})
        for (int shift = 24; shift >= 0; shift -= 8) { legacy *= 899433627U; legacy ^= (value >> shift) & 255U; }
    document["schema"] = 2;
    document.remove("lane_count");
    document.remove("input_port");
    document["checksum"] = legacy;
    EXPECT(StateCodec::decode(document, restored));
    EXPECT(restored.lane_count == 4 && restored.input_port == 0);
}
