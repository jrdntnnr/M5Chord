#pragma once

#include "midi/MidiEvent.h"
#include "midi/MidiParser.h"

#include <cstddef>
#include <cstdint>

namespace midibrain {

class BleMidiDecoder {
public:
    using EventCallback = void (*)(void*, const MidiEvent&);

    std::size_t decode(const uint8_t* data, std::size_t size, uint64_t timestampUs, EventCallback callback, void* context);
    void reset();

private:
    static uint8_t dataLength(uint8_t status);

    MidiParser parser_{};
    uint8_t running_status_{0};
    bool system_exclusive_{false};
};

}
