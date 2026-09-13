#include "midi/MidiParser.h"

namespace midibrain {

MidiEvent MidiParser::parseMessage(uint8_t status, uint8_t data1, uint8_t data2, uint64_t timestampUs, uint8_t cable) const {
    const uint8_t command = status & 0xf0U;
    const uint8_t channel = status & 0x0fU;
    MidiEvent event{MidiType::Unknown, channel, data1, data2, 0, timestampUs, cable};
    switch (command) {
        case 0x80: event.type = MidiType::NoteOff; break;
        case 0x90: event.type = data2 == 0 ? MidiType::NoteOff : MidiType::NoteOn; break;
        case 0xa0: event.type = MidiType::PolyAftertouch; break;
        case 0xb0: event.type = MidiType::ControlChange; break;
        case 0xc0: event.type = MidiType::ProgramChange; break;
        case 0xd0: event.type = MidiType::ChannelPressure; break;
        case 0xe0:
            event.type = MidiType::PitchBend;
            event.value14 = static_cast<uint16_t>(data1 | (data2 << 7U));
            break;
        case 0xf0:
            if (status == 0xf8) event.type = MidiType::Clock;
            else if (status == 0xfa) event.type = MidiType::Start;
            else if (status == 0xfb) event.type = MidiType::Continue;
            else if (status == 0xfc) event.type = MidiType::Stop;
            else if (status == 0xf2) {
                event.type = MidiType::SongPosition;
                event.value14 = static_cast<uint16_t>(data1 | (data2 << 7U));
            }
            break;
        default: break;
    }
    return event;
}

MidiEvent MidiParser::parseUsbPacket(const uint8_t packet[4], uint64_t timestampUs) const {
    return parseMessage(packet[1], packet[2], packet[3], timestampUs, packet[0] >> 4U);
}

std::size_t MidiParser::serialize(const MidiEvent& event, uint8_t output[3]) const {
    uint8_t status = 0;
    std::size_t size = 0;
    switch (event.type) {
        case MidiType::NoteOff: status = 0x80; size = 3; break;
        case MidiType::NoteOn: status = 0x90; size = 3; break;
        case MidiType::PolyAftertouch: status = 0xa0; size = 3; break;
        case MidiType::ControlChange: status = 0xb0; size = 3; break;
        case MidiType::ProgramChange: status = 0xc0; size = 2; break;
        case MidiType::ChannelPressure: status = 0xd0; size = 2; break;
        case MidiType::PitchBend: status = 0xe0; size = 3; break;
        case MidiType::Clock: output[0] = 0xf8; return 1;
        case MidiType::Start: output[0] = 0xfa; return 1;
        case MidiType::Continue: output[0] = 0xfb; return 1;
        case MidiType::Stop: output[0] = 0xfc; return 1;
        case MidiType::SongPosition: output[0] = 0xf2; size = 3; break;
        default: return 0;
    }
    output[0] = static_cast<uint8_t>(status | (event.channel & 0x0fU));
    if (event.type == MidiType::PitchBend || event.type == MidiType::SongPosition) {
        output[1] = event.value14 & 0x7fU;
        output[2] = (event.value14 >> 7U) & 0x7fU;
    } else {
        output[1] = event.data1 & 0x7fU;
        output[2] = event.data2 & 0x7fU;
    }
    return size;
}

}
