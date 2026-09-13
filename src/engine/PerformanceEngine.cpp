#include "engine/PerformanceEngine.h"

#include "engine/VoicingEngine.h"

#include <algorithm>
#include <array>

namespace midibrain {

PerformanceEngine::PerformanceEngine(uint32_t seed) : random_state_(seed == 0 ? 1 : seed) {}

uint32_t PerformanceEngine::random() {
    uint32_t value = random_state_;
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    random_state_ = value;
    return value;
}

int PerformanceEngine::jitter(int maximum) {
    if (maximum <= 0) {
        return 0;
    }
    return static_cast<int>(random() % static_cast<uint32_t>(maximum * 2 + 1)) - maximum;
}

uint64_t PerformanceEngine::stepDurationUs(uint16_t bpm, Division division) const {
    const uint64_t quarter = 60000000ULL / std::clamp<uint16_t>(bpm, 30, 300);
    switch (division) {
        case Division::Quarter: return quarter;
        case Division::Eighth: return quarter / 2;
        case Division::EighthTriplet: return quarter / 3;
        case Division::Sixteenth: return quarter / 4;
        case Division::SixteenthTriplet: return quarter / 6;
        case Division::ThirtySecond: return quarter / 8;
    }
    return quarter / 4;
}

NoteList PerformanceEngine::poolFor(const NoteList& notes, PerformanceMode mode) const {
    const bool twoOctaves = mode == PerformanceMode::StrumTwoOctaves || mode == PerformanceMode::ArpTwoOctaves;
    const uint8_t octaves = mode == PerformanceMode::Harp ? 3 : twoOctaves ? 2 : 1;
    return VoicingEngine{}.expandOctaves(notes, octaves);
}

std::size_t PerformanceEngine::noteIndex(std::size_t count, uint32_t step, Direction direction) {
    if (count < 2) {
        return 0;
    }
    if (direction == Direction::Down) {
        return count - 1 - step % count;
    }
    if (direction == Direction::UpDown) {
        const std::size_t cycle = count * 2 - 2;
        const std::size_t position = step % cycle;
        return position < count ? position : cycle - position;
    }
    if (direction == Direction::Random) {
        return random() % count;
    }
    return step % count;
}

bool PerformanceEngine::trigger(const NoteList& notes, uint8_t channel, uint8_t velocity, uint64_t startUs, const VoiceId& owner, const PerformanceConfig& config, MidiScheduler& scheduler) {
    const NoteList pool = poolFor(notes, config.mode);
    if (pool.empty()) {
        return false;
    }
    if (config.mode == PerformanceMode::Arp || config.mode == PerformanceMode::ArpTwoOctaves || config.mode == PerformanceMode::Pattern) {
        return scheduleArpStep(pool, channel, velocity, startUs, 0, owner, config, scheduler);
    }
    const uint64_t spacing = config.mode == PerformanceMode::Block ? 0 : config.mode == PerformanceMode::Harp ? 18000ULL : static_cast<uint64_t>(config.strum_interval_ms) * 1000ULL;
    for (std::size_t i = 0; i < pool.size(); ++i) {
        if (config.mode == PerformanceMode::Harp && scheduler.size() + 2 > MidiScheduler::Capacity) return false;
        const std::size_t index = config.direction == Direction::Down ? pool.size() - 1 - i : i;
        int64_t due = static_cast<int64_t>(startUs + i * spacing);
        int adjustedVelocity = velocity;
        if (config.mode == PerformanceMode::Slop) {
            const int timingMaximum = 8000 * config.slop_amount / 100;
            const int velocityMaximum = 5 * config.slop_amount / 100;
            due = std::max<int64_t>(startUs, due + jitter(timingMaximum));
            adjustedVelocity = std::clamp<int>(velocity + jitter(velocityMaximum), 1, 127);
        }
        if (!scheduler.schedule(static_cast<uint64_t>(due), owner, StreamId::Performance, MidiEvent::noteOn(channel, pool[index], static_cast<uint8_t>(adjustedVelocity), static_cast<uint64_t>(due)))) {
            return false;
        }
        if (config.mode == PerformanceMode::Harp) {
            const uint64_t off = static_cast<uint64_t>(due) + 350000ULL;
            if (!scheduler.schedule(off, owner, StreamId::Performance, MidiEvent::noteOff(channel, pool[index], 0, off))) {
                return false;
            }
        }
    }
    return true;
}

bool PerformanceEngine::scheduleArpStep(const NoteList& notes, uint8_t channel, uint8_t velocity, uint64_t startUs, uint32_t step, const VoiceId& owner, const PerformanceConfig& config, MidiScheduler& scheduler) {
    if (notes.empty() || scheduler.size() + 2 > MidiScheduler::Capacity) {
        return false;
    }
    static constexpr std::array<uint8_t, 6> pattern{{0, 1, 3, 2, 1, 2}};
    std::size_t index = noteIndex(notes.size(), step, config.direction);
    if (config.mode == PerformanceMode::Pattern) {
        index = pattern[step % pattern.size()] % notes.size();
    }
    const uint64_t duration = stepDurationUs(config.bpm, config.division);
    const uint64_t off = startUs + duration * std::clamp<uint8_t>(config.gate_percent, 1, 100) / 100;
    return scheduler.schedule(startUs, owner, StreamId::Performance, MidiEvent::noteOn(channel, notes[index], velocity, startUs))
        && scheduler.schedule(off, owner, StreamId::Performance, MidiEvent::noteOff(channel, notes[index], 0, off));
}

}
