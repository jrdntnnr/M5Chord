#pragma once

#include "common/FixedList.h"
#include "midi/MidiEvent.h"
#include "scheduler/MidiScheduler.h"
#include <array>
#include <cstdint>

namespace midibrain {

enum class LoopMode : uint8_t { Stopped, Recording, Playing, Overdub };

struct LoopEntry {
    uint32_t tick{0};
    uint32_t duration{0};
    MidiType type{MidiType::Unknown};
    uint8_t channel{0};
    uint8_t data1{0};
    uint8_t data2{0};
    uint16_t value14{0};
    uint16_t layer{0};
};

class MidiLooper {
public:
    static constexpr uint32_t TicksPerQuarter = 96;
    static constexpr std::size_t Capacity = 512;
    static constexpr uint32_t MaximumTicks = TicksPerQuarter * 4U * 1024U;
    void record(uint64_t nowUs, uint16_t bpm, uint8_t bars, uint16_t grid);
    void play(uint64_t nowUs);
    void stop(uint64_t nowUs);
    void clear();
    void overdub(uint64_t nowUs);
    void undo(uint64_t nowUs);
    void capture(const MidiEvent& event, uint64_t nowUs);
    bool tick(uint64_t nowUs, MidiScheduler& scheduler);
    LoopMode mode() const;
    uint32_t length() const;
    uint16_t bpm() const;
    uint32_t dropped() const;
    uint16_t sustainChannels() const;
    const FixedList<LoopEntry, Capacity>& entries() const;
    bool replace(const FixedList<LoopEntry, Capacity>& entries, uint32_t ticks, uint16_t bpm);
private:
    struct Pending {
        bool active{false};
        uint8_t channel{0};
        uint8_t note{0};
        uint16_t entry{0};
        uint64_t at{0};
    };
    void finishNotes(uint64_t nowUs);
    uint64_t ticksAt(uint64_t nowUs) const;
    uint64_t micros(uint64_t tick) const;
    FixedList<LoopEntry, Capacity> entries_{};
    std::array<Pending, 128> pending_{};
    LoopMode mode_{LoopMode::Stopped};
    uint64_t origin_us_{0};
    uint64_t cycle_{0};
    uint32_t length_ticks_{0};
    uint32_t target_ticks_{0};
    uint16_t bpm_{100};
    uint16_t grid_{0};
    uint16_t layer_{0};
    uint32_t dropped_{0};
    std::array<bool, Capacity> emitted_{};
};

}
