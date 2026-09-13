#include "transport/DinMidiSink.h"
#include "hardware/CardputerHardware.h"

#include <algorithm>

namespace midibrain {

#ifdef ARDUINO
DinMidiSink::DinMidiSink(HardwareSerial& serial) : serial_(&serial) {}
#endif

bool DinMidiSink::begin() {
#ifdef ARDUINO
    serial_->setRxBufferSize(1024);
    serial_->begin(31250, SERIAL_8N1, CardputerPins::MidiRx, CardputerPins::MidiTx);
    return true;
#else
    return false;
#endif
}

bool DinMidiSink::send(const MidiEvent& event) {
    uint8_t bytes[3]{};
    const std::size_t size = parser_.serialize(event, bytes);
    if (size == 0) {
        ++dropped_;
        return false;
    }
    const bool critical = event.type == MidiType::NoteOff
        || event.type == MidiType::Stop
        || (event.type == MidiType::ControlChange && (event.data1 == 64 || event.data1 == 120 || event.data1 == 121 || event.data1 == 123));
    const bool coalescible = event.type == MidiType::PitchBend || event.type == MidiType::ChannelPressure
        || (event.type == MidiType::ControlChange && (event.data1 == 1 || event.data1 == 7 || event.data1 == 10 || event.data1 == 11 || event.data1 == 74));
    if (coalescible && queue_size_) {
        auto& last = queue_[queue_size_ - 1];
        if (last.bytes[0] == bytes[0] && (event.type != MidiType::ControlChange || last.bytes[1] == bytes[1])) {
            std::copy(bytes, bytes + size, last.bytes.begin());
            return true;
        }
    }
    if (queue_size_ == Capacity) {
        if (!critical && event.type != MidiType::NoteOn) {
            ++dropped_;
            return false;
        }
        std::size_t victim = Capacity;
        for (std::size_t i = 0; i < queue_size_; ++i) {
            if ((critical && !queue_[i].critical) || (event.type == MidiType::NoteOn && queue_[i].coalescible)) {
                victim = i;
                break;
            }
        }
        if (victim == Capacity) {
            ++dropped_;
            recovery_required_ = recovery_required_ || critical;
            return false;
        }
        for (std::size_t i = victim + 1; i < queue_size_; ++i) queue_[i - 1] = queue_[i];
        --queue_size_;
        ++dropped_;
    }
    QueuedMessage& message = queue_[queue_size_++];
    std::copy(bytes, bytes + size, message.bytes.begin());
    message.size = static_cast<uint8_t>(size);
    message.critical = critical;
    message.coalescible = coalescible;
    high_water_ = std::max(high_water_, queue_size_);
    return true;
}

bool DinMidiSink::takeRecovery() {
    if (!recovery_required_) return false;
    recovery_required_ = false;
    queue_size_ = 0;
    return true;
}

void DinMidiSink::poll() {
#ifdef ARDUINO
    while (queue_size_ > 0 && serial_->availableForWrite() >= queue_[0].size) {
        serial_->write(queue_[0].bytes.data(), queue_[0].size);
        for (std::size_t i = 1; i < queue_size_; ++i) queue_[i - 1] = queue_[i];
        --queue_size_;
        ++sent_;
    }
#endif
}

void DinMidiSink::discardPending() {
    std::size_t write = 0;
    for (std::size_t read = 0; read < queue_size_; ++read) {
        if (queue_[read].critical) queue_[write++] = queue_[read];
    }
    queue_size_ = write;
}

uint32_t DinMidiSink::sent() const { return sent_; }
uint32_t DinMidiSink::dropped() const { return dropped_; }
std::size_t DinMidiSink::queued() const { return queue_size_; }
std::size_t DinMidiSink::highWater() const { return high_water_; }

}
