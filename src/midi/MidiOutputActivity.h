#pragma once

#include "midi/MidiEvent.h"
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>

namespace midibrain {

struct OutputActivitySnapshot {
    std::bitset<128> gates{};
    std::bitset<128> attacks{};
    uint8_t latest{128};
};

class MidiOutputActivity {
public:
    static constexpr uint64_t AttackUs = 50000;
    void observe(const MidiEvent& event, uint64_t dispatchedUs) {
        if (event.channel >= gates_.size() || event.data1 >= 128) return;
        if (event.type == MidiType::NoteOn && event.data2) {
            gates_[event.channel].set(event.data1);
            attacks_[next_] = {dispatchedUs, event.channel, event.data1, true};
            next_ = (next_ + 1) % attacks_.size();
        } else if (event.type == MidiType::NoteOff || event.type == MidiType::NoteOn) {
            gates_[event.channel].reset(event.data1);
        } else if (event.type == MidiType::ControlChange && (event.data1 == 120 || event.data1 == 123)) {
            gates_[event.channel].reset();
            for (auto& attack : attacks_) if (attack.channel == event.channel) attack.valid = false;
        }
    }
    OutputActivitySnapshot snapshot(int channel, uint64_t nowUs) const {
        OutputActivitySnapshot result;
        if (channel < -1 || channel >= 16) return result;
        if (channel < 0) for (const auto& gates : gates_) result.gates |= gates;
        else result.gates = gates_[static_cast<std::size_t>(channel)];
        for (std::size_t i = 0; i < attacks_.size(); ++i) {
            const auto& attack = attacks_[(next_ + i) % attacks_.size()];
            if (!attack.valid || (channel >= 0 && attack.channel != channel)) continue;
            if (nowUs < attack.at_us || nowUs - attack.at_us >= AttackUs) continue;
            result.attacks.set(attack.note);
            result.latest = attack.note;
        }
        return result;
    }
    void clear() {
        for (auto& gates : gates_) gates.reset();
        for (auto& attack : attacks_) attack.valid = false;
        next_ = 0;
    }

private:
    struct Attack {
        uint64_t at_us{0};
        uint8_t channel{0};
        uint8_t note{0};
        bool valid{false};
    };
    std::array<std::bitset<128>, 16> gates_{};
    std::array<Attack, 64> attacks_{};
    std::size_t next_{0};
};

}
