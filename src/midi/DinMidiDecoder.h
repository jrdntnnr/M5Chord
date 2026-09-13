#pragma once
#include "midi/MidiParser.h"

namespace midibrain {

class DinMidiDecoder {
public:
    using Callback = void (*)(void*, const MidiEvent&);
    void reset() { *this = DinMidiDecoder{}; }
    void feed(uint8_t byte, uint64_t nowUs, Callback callback, void* context) {
        last_byte_us_ = nowUs;
        if (byte >= 0xf8) {
            if (byte == 0xfe) sensing_ = true;
            else if (byte == 0xff) reset_pending_ = true;
            else emit(byte, 0, 0, nowUs, callback, context);
            return;
        }
        if (byte & 0x80) {
            count_ = 0;
            sysex_ = byte == 0xf0;
            status_ = byte;
            if (byte < 0xf0) {
                running_ = byte;
                needed_ = (byte & 0xe0) == 0xc0 ? 1 : 2;
            } else {
                running_ = 0;
                needed_ = byte == 0xf2 ? 2 : byte == 0xf1 || byte == 0xf3 ? 1 : 0;
            }
            return;
        }
        if (sysex_ || !needed_) return;
        data_[count_++] = byte;
        if (count_ != needed_) return;
        emit(status_, data_[0], needed_ == 2 ? data_[1] : 0, nowUs, callback, context);
        count_ = 0;
        status_ = running_;
        if (!running_) needed_ = 0;
    }
    bool takeRecovery(uint64_t nowUs) {
        if (!reset_pending_ && !(sensing_ && nowUs >= last_byte_us_ && nowUs - last_byte_us_ > 300000)) return false;
        reset();
        return true;
    }
private:
    void emit(uint8_t status, uint8_t a, uint8_t b, uint64_t nowUs, Callback callback, void* context) {
        const auto event = MidiParser{}.parseMessage(status, a, b, nowUs);
        if (event.type != MidiType::Unknown && callback) callback(context, event);
    }
    uint64_t last_byte_us_{0};
    uint8_t status_{0};
    uint8_t running_{0};
    uint8_t needed_{0};
    uint8_t count_{0};
    uint8_t data_[2]{};
    bool sysex_{false};
    bool sensing_{false};
    bool reset_pending_{false};
};

}
