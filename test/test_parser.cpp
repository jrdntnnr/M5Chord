#include "TestSupport.h"
#include "midi/MidiParser.h"

using namespace midibrain;

void testParser() {
    MidiParser parser;
    const uint8_t noteOn[4]{0x09, 0x90, 60, 100};
    const auto parsed = parser.parseUsbPacket(noteOn, 42);
    EXPECT(parsed.type == MidiType::NoteOn);
    EXPECT(parsed.data1 == 60 && parsed.data2 == 100 && parsed.timestamp_us == 42);
    const uint8_t zeroVelocity[4]{0x09, 0x91, 61, 0};
    EXPECT(parser.parseUsbPacket(zeroVelocity, 0).type == MidiType::NoteOff);
    const uint8_t bend[4]{0x0e, 0xe2, 0, 64};
    EXPECT(parser.parseUsbPacket(bend, 0).value14 == 8192);
    const auto direct = parser.parseMessage(0xb3, 7, 96, 91);
    EXPECT(direct.type == MidiType::ControlChange);
    EXPECT(direct.channel == 3 && direct.data1 == 7 && direct.data2 == 96 && direct.timestamp_us == 91);
    uint8_t output[3]{};
    EXPECT(parser.serialize(parsed, output) == 3);
    EXPECT(output[0] == 0x90 && output[1] == 60 && output[2] == 100);
}
