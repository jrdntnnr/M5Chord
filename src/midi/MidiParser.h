#pragma once

#include "midi/MidiEvent.h"

#include <cstddef>
#include <cstdint>

namespace midibrain {

class MidiParser {
public:
    MidiEvent parseMessage(uint8_t status, uint8_t data1, uint8_t data2, uint64_t timestampUs, uint8_t cable = 0) const;
    MidiEvent parseUsbPacket(const uint8_t packet[4], uint64_t timestampUs) const;
    std::size_t serialize(const MidiEvent& event, uint8_t output[3]) const;
};

}
