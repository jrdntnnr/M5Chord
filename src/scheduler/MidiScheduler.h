#pragma once

#include "common/FixedList.h"
#include "midi/MidiEvent.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace midibrain {

struct ScheduledMidiEvent {
    uint64_t due_us{0};
    uint32_t sequence{0};
    VoiceId owner{};
    StreamId stream{StreamId::Performance};
    MidiEvent event{};
};

struct SchedulerStats {
    uint32_t dropped{0};
    uint32_t late_count{0};
    uint32_t last_late_us{0};
    uint32_t max_late_us{0};
    uint64_t total_late_us{0};
    std::size_t high_water{0};
};

class MidiScheduler {
public:
    static constexpr std::size_t Capacity = 512;

    bool schedule(uint64_t dueUs, const VoiceId& owner, StreamId stream, const MidiEvent& event);
    bool popDue(uint64_t nowUs, ScheduledMidiEvent& event);
    std::size_t cancel(const VoiceId& owner);
    std::size_t cancel(StreamId stream);
    void clear();
    std::size_t size() const;
    bool empty() const;
    const SchedulerStats& stats() const;

private:
    bool earlier(const ScheduledMidiEvent& left, const ScheduledMidiEvent& right) const;
    void siftUp(std::size_t index);
    void siftDown(std::size_t index);
    void rebuild();

    std::array<ScheduledMidiEvent, Capacity> heap_{};
    std::size_t size_{0};
    uint32_t next_sequence_{0};
    SchedulerStats stats_{};
};

}

