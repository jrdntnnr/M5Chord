#include "TestSupport.h"
#include "midi/BleMidiDecoder.h"
#include "transport/BleMidiSubscription.h"
#include "transport/BleMidiMtuGuard.h"

using namespace midibrain;

namespace {

void collect(void* context, const MidiEvent& event) {
    static_cast<FixedList<MidiEvent, 16>*>(context)->push_back(event);
}

}

void testBleMidi() {
    BleMidiMtuGuard mtu;
    EXPECT(mtu.connected(255, 0));
    EXPECT(!mtu.connected(255, 0));
    mtu.reset();
    EXPECT(!mtu.connected(0, 0));
    EXPECT(!mtu.connected(0, 0));
    mtu.reset();
    EXPECT(mtu.connected(0, 1));
    EXPECT(!mtu.connected(0, 1));
    using Operation = BleMidiSubscription::Operation;
    BleMidiSubscription subscription;
    EXPECT(subscription.poll(9000000) == Operation::None);
    subscription.start(100);
    EXPECT(!subscription.ready());
    EXPECT(subscription.poll(500099) == Operation::None);
    EXPECT(subscription.poll(500100) == Operation::Read);
    subscription.subscribed(true, 500101);
    subscription.registered(true, 500101);
    EXPECT(!subscription.ready());
    EXPECT(subscription.poll(500199) == Operation::None);
    subscription.readComplete(true, 500200);
    EXPECT(subscription.poll(500200) == Operation::Register);
    subscription.registered(true, 500200);
    EXPECT(subscription.poll(500200) == Operation::Enable);
    EXPECT(!subscription.ready());
    subscription.subscribed(false, 500300);
    EXPECT(subscription.poll(1000299) == Operation::None);
    EXPECT(subscription.poll(1000300) == Operation::Enable);
    subscription.subscribed(true, 1000400);
    EXPECT(subscription.ready());
    EXPECT(subscription.poll(3000399) == Operation::None);
    EXPECT(subscription.poll(3000400) == Operation::None);
    EXPECT(subscription.ready());
    subscription.subscribed(true, 3000500);
    EXPECT(subscription.poll(3100499) == Operation::None);
    EXPECT(subscription.poll(3100500) == Operation::None);
    subscription.subscribed(true, 3100600);
    EXPECT(subscription.poll(6000000) == Operation::None);
    EXPECT(subscription.poll(3600000000ULL) == Operation::None);
    EXPECT(subscription.ready());

    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    subscription.readComplete(true, 500000);
    EXPECT(subscription.poll(500000) == Operation::Register);
    subscription.registered(true, 500000);
    EXPECT(subscription.poll(500000) == Operation::Enable);
    subscription.subscribed(true, 500000);
    EXPECT(subscription.poll(3600000000ULL) == Operation::None);
    EXPECT(subscription.ready());
    subscription.reset();
    EXPECT(!subscription.ready());
    subscription.subscribed(true, 600000);
    subscription.registered(true, 600000);
    subscription.readComplete(true, 600000);
    EXPECT(subscription.poll(3600000000ULL) == Operation::None);

    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    EXPECT(subscription.poll(2499999) == Operation::None);
    EXPECT(subscription.poll(2500000) == Operation::Disconnect);
    EXPECT(subscription.poll(2500001) == Operation::None);
    EXPECT(!subscription.ready());

    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    subscription.readComplete(true, 500000);
    EXPECT(subscription.poll(500000) == Operation::Register);
    subscription.registered(false, 500001);
    EXPECT(subscription.poll(500001) == Operation::Disconnect);

    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    subscription.readComplete(true, 500000);
    EXPECT(subscription.poll(500000) == Operation::Register);
    subscription.registered(true, 500000);
    for (uint64_t attempt = 1; attempt <= 3; ++attempt) {
        EXPECT(subscription.poll(attempt * 500000) == Operation::Enable);
        subscription.subscribed(false, attempt * 500000);
        EXPECT(!subscription.ready());
    }
    EXPECT(subscription.poll(2000000) == Operation::Disconnect);

    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    subscription.readComplete(true, 500000);
    EXPECT(subscription.poll(500000) == Operation::Register);
    subscription.registered(true, 500000);
    EXPECT(subscription.poll(500000) == Operation::Enable);
    EXPECT(subscription.poll(2500000) == Operation::Disconnect);

    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    subscription.readComplete(false, 500000);
    EXPECT(subscription.poll(500000) == Operation::Disconnect);
    EXPECT(!subscription.ready());

    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    subscription.readComplete(true, 500000);
    EXPECT(subscription.poll(500000) == Operation::Register);
    EXPECT(subscription.poll(2500000) == Operation::Disconnect);

    BleMidiDecoder decoder;
    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    subscription.readComplete(false, 500001, true);
    EXPECT(subscription.poll(500001) == Operation::Encrypt);
    EXPECT(subscription.poll(500002) == Operation::None);
    subscription.encrypted(true, 600000);
    EXPECT(subscription.poll(600000) == Operation::Read);
    subscription.readComplete(true, 600001);
    EXPECT(subscription.poll(600001) == Operation::Register);
    subscription.registered(true, 600002);
    EXPECT(subscription.poll(600002) == Operation::Enable);
    subscription.subscribed(true, 600003);
    EXPECT(subscription.ready());
    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    subscription.readComplete(false, 500000, true);
    EXPECT(subscription.poll(500000) == Operation::Encrypt);
    EXPECT(subscription.poll(10500000) == Operation::Disconnect);
    subscription.start(0);
    EXPECT(subscription.poll(500000) == Operation::Read);
    subscription.readComplete(false, 500000, true);
    EXPECT(subscription.poll(500000) == Operation::Encrypt);
    subscription.encrypted(true, 500001);
    EXPECT(subscription.poll(500001) == Operation::Read);
    subscription.readComplete(false, 500002, true);
    EXPECT(subscription.poll(500002) == Operation::Disconnect);
    FixedList<MidiEvent, 16> events;
    const uint8_t notes[]{0x80, 0x81, 0x90, 60, 100, 0x82, 61, 110, 0x83, 60, 0};
    EXPECT(decoder.decode(notes, sizeof(notes), 1000, collect, &events) == 3);
    EXPECT(events.size() == 3);
    EXPECT(events[0].type == MidiType::NoteOn && events[0].data1 == 60 && events[0].data2 == 100);
    EXPECT(events[1].type == MidiType::NoteOn && events[1].data1 == 61 && events[1].data2 == 110);
    EXPECT(events[2].type == MidiType::NoteOff && events[2].data1 == 60);
    EXPECT(events[0].timestamp_us == 1000);

    events.clear();
    const uint8_t controls[]{0xa1, 0x90, 0xb2, 1, 64, 0x91, 0xc2, 12, 0x92, 0xe2, 0, 64};
    EXPECT(decoder.decode(controls, sizeof(controls), 2000, collect, &events) == 3);
    EXPECT(events[0].type == MidiType::ControlChange && events[0].channel == 2);
    EXPECT(events[1].type == MidiType::ProgramChange && events[1].data1 == 12);
    EXPECT(events[2].type == MidiType::PitchBend && events[2].value14 == 8192);

    events.clear();
    const uint8_t transport[]{0x80, 0x80, 0xfa, 0x81, 0xf8, 0x82, 0xfc};
    EXPECT(decoder.decode(transport, sizeof(transport), 3000, collect, &events) == 3);
    EXPECT(events[0].type == MidiType::Start);
    EXPECT(events[1].type == MidiType::Clock);
    EXPECT(events[2].type == MidiType::Stop);

    decoder.reset();
    events.clear();
    const uint8_t noStatus[]{0x80, 0x80, 60, 100};
    EXPECT(decoder.decode(noStatus, sizeof(noStatus), 0, collect, &events) == 0);
    EXPECT(events.empty());
}
