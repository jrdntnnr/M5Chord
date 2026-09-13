#pragma once

#include "app/App.h"
#include <algorithm>

namespace midibrain {

class LiveKeyboard {
public:
    void update(const App& app, uint64_t nowUs) {
        const auto& state = app.state();
        output_ = app.outputActivity().snapshot(state.mode == EngineMode::Bypass ? -1 : state.routing.performance_channel, nowUs);
        int low = 128;
        int high = -1;
        NoteList pool = app.lastChord();
        if (state.mode != EngineMode::Bypass) {
            const unsigned octaves = state.performance.mode == PerformanceMode::Harp ? 3
                : state.performance.mode == PerformanceMode::StrumTwoOctaves || state.performance.mode == PerformanceMode::ArpTwoOctaves ? 2 : 1;
            pool = VoicingEngine{}.expandOctaves(pool, octaves);
        }
        for (const auto note : pool) { low = std::min<int>(low, note); high = std::max<int>(high, note); }
        for (unsigned note = 0; note < 128; ++note) if (output_.gates[note] || output_.attacks[note]) {
            low = std::min<int>(low, note);
            high = std::max<int>(high, note);
        }
        if (high < 0) { low = 60; high = 71; }
        base_ = static_cast<uint8_t>(std::min(108, low / 12 * 12));
        span_ = high - base_ < 24 ? 24 : high - base_ < 36 ? 36 : 48;
        if (high >= base_ + span_ && output_.latest < 128) base_ = static_cast<uint8_t>(std::min(108, output_.latest / 12 * 12));
    }
    uint8_t base() const { return base_; }
    uint8_t span() const { return span_; }
    bool gate(unsigned note) const { return note < 128 && output_.gates[note]; }
    bool attack(unsigned note) const { return note < 128 && output_.attacks[note]; }
    uint8_t latest() const { return output_.latest; }

private:
    OutputActivitySnapshot output_{};
    uint8_t base_{60};
    uint8_t span_{24};
};

}
