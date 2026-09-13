#pragma once

#include "engine/MusicTypes.h"
#include "midi/MidiEvent.h"
#include "scheduler/MidiScheduler.h"

#include <cstdint>

namespace midibrain {

struct PerformanceConfig {
    PerformanceMode mode{PerformanceMode::Block};
    Direction direction{Direction::Up};
    Division division{Division::Sixteenth};
    uint16_t bpm{100};
    uint8_t gate_percent{75};
    uint16_t strum_interval_ms{25};
    uint8_t slop_amount{20};
};

class PerformanceEngine {
public:
    explicit PerformanceEngine(uint32_t seed = 0x8f31a2c7U);
    bool trigger(const NoteList& notes, uint8_t channel, uint8_t velocity, uint64_t startUs, const VoiceId& owner, const PerformanceConfig& config, MidiScheduler& scheduler);
    bool scheduleArpStep(const NoteList& notes, uint8_t channel, uint8_t velocity, uint64_t startUs, uint32_t step, const VoiceId& owner, const PerformanceConfig& config, MidiScheduler& scheduler);
    uint64_t stepDurationUs(uint16_t bpm, Division division) const;

private:
    uint32_t random();
    int jitter(int maximum);
    std::size_t noteIndex(std::size_t count, uint32_t step, Direction direction);
    NoteList poolFor(const NoteList& notes, PerformanceMode mode) const;

    uint32_t random_state_;
};

}

