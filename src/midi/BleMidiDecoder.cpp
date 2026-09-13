#include "midi/BleMidiDecoder.h"

namespace midibrain {

uint8_t BleMidiDecoder::dataLength(uint8_t status) {
    const uint8_t command = status & 0xf0U;
    if (command >= 0x80U && command <= 0xb0U) return 2;
    if (command == 0xc0U || command == 0xd0U) return 1;
    if (command == 0xe0U) return 2;
    if (status == 0xf1U || status == 0xf3U) return 1;
    if (status == 0xf2U) return 2;
    return 0;
}

std::size_t BleMidiDecoder::decode(const uint8_t* data, std::size_t size, uint64_t timestampUs, EventCallback callback, void* context) {
    if (data == nullptr || size < 2 || (data[0] & 0x80U) == 0) return 0;
    std::size_t emitted = 0;
    std::size_t offset = 1;
    while (offset < size) {
        if (system_exclusive_ && (data[offset] & 0x80U) == 0) {
            ++offset;
            continue;
        }
        if ((data[offset] & 0x80U) == 0) break;
        ++offset;
        if (offset >= size) break;
        uint8_t status = data[offset];
        if ((status & 0x80U) != 0) {
            ++offset;
            if (status == 0xf0U) {
                system_exclusive_ = true;
                running_status_ = 0;
                continue;
            }
            if (status == 0xf7U) {
                system_exclusive_ = false;
                running_status_ = 0;
                continue;
            }
            if (status < 0xf0U) running_status_ = status;
            else if (status < 0xf8U) running_status_ = 0;
        } else {
            if (running_status_ == 0) break;
            status = running_status_;
        }
        const uint8_t required = dataLength(status);
        uint8_t messageData[2]{};
        for (uint8_t index = 0; index < required; ++index) {
            if (offset >= size || (data[offset] & 0x80U) != 0) return emitted;
            messageData[index] = data[offset++];
        }
        const MidiEvent event = parser_.parseMessage(status, messageData[0], messageData[1], timestampUs);
        if (event.type != MidiType::Unknown && callback != nullptr) {
            callback(context, event);
            ++emitted;
        }
    }
    return emitted;
}

void BleMidiDecoder::reset() {
    running_status_ = 0;
    system_exclusive_ = false;
}

}
