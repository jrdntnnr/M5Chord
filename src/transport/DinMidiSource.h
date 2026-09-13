#pragma once
#include "midi/DinMidiDecoder.h"
#include <atomic>
#ifdef ARDUINO
#include <HardwareSerial.h>
#include <esp_timer.h>
#endif

namespace midibrain {

class DinMidiSource {
public:
    using Callback = DinMidiDecoder::Callback;
    using RecoveryCallback = void (*)(void*);
    DinMidiSource(Callback callback, RecoveryCallback recovery, void* context)
        : callback_(callback), recovery_(recovery), context_(context) {}
#ifdef ARDUINO
    void begin(HardwareSerial& serial) {
        serial_ = &serial;
        serial_->onReceiveError([this](hardwareSerial_error_t) { failed_.store(true); });
    }
    void poll() {
        const uint64_t nowUs = esp_timer_get_time();
        if (failed_.exchange(false)) {
            ++errors_;
            decoder_.reset();
            while (serial_->available()) serial_->read();
            if (recovery_) recovery_(context_);
        }
        for (unsigned i = 0; i < 128 && serial_->available(); ++i) {
            ++bytes_;
            decoder_.feed(static_cast<uint8_t>(serial_->read()), esp_timer_get_time(), decoded, this);
        }
        if (decoder_.takeRecovery(nowUs)) {
            ++errors_;
            if (recovery_) recovery_(context_);
        }
    }
#endif
    uint32_t bytes() const { return bytes_; }
    uint32_t events() const { return events_; }
    uint32_t errors() const { return errors_; }
private:
    static void decoded(void* context, const MidiEvent& event) {
        auto* source = static_cast<DinMidiSource*>(context);
        ++source->events_;
        if (source->callback_) source->callback_(source->context_, event);
    }
#ifdef ARDUINO
    HardwareSerial* serial_{nullptr};
#endif
    Callback callback_;
    RecoveryCallback recovery_;
    void* context_;
    DinMidiDecoder decoder_{};
    std::atomic<bool> failed_{false};
    uint32_t bytes_{0};
    uint32_t events_{0};
    uint32_t errors_{0};
};

}
