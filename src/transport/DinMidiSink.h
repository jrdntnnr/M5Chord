#pragma once

#include "midi/MidiParser.h"
#include "transport/MidiSink.h"

#include <array>
#include <cstddef>

#ifdef ARDUINO
#include <HardwareSerial.h>
#endif

namespace midibrain {

class DinMidiSink final : public MidiSink {
public:
#ifdef ARDUINO
    explicit DinMidiSink(HardwareSerial& serial);
#else
    DinMidiSink() = default;
#endif
    bool begin();
    bool send(const MidiEvent& event) override;
    void discardPending() override;
    void poll();
    bool takeRecovery();
    uint32_t sent() const;
    uint32_t dropped() const;
    std::size_t queued() const;
    std::size_t highWater() const;

private:
#ifdef ARDUINO
    HardwareSerial* serial_{nullptr};
#endif
    MidiParser parser_{};
    struct QueuedMessage {
        std::array<uint8_t, 3> bytes{};
        uint8_t size{0};
        bool critical{false};
        bool coalescible{false};
    };
    static constexpr std::size_t Capacity = 384;
    std::array<QueuedMessage, Capacity> queue_{};
    std::size_t queue_size_{0};
    std::size_t high_water_{0};
    uint32_t sent_{0};
    uint32_t dropped_{0};
    bool recovery_required_{false};
};

}
