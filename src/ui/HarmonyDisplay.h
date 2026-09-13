#pragma once

#include "app/App.h"
#include <array>
#include <cstdint>

namespace midibrain {

enum class PadDisplayState : uint8_t { Off, Selected, Held, Disabled };

class HarmonyDisplay {
public:
    static constexpr uint64_t LingerUs = 1200000;
    static constexpr uint64_t FadeUs = 500000;
    void update(const App& app, uint64_t nowUs);
    const char* keyLabel() const { return key_label_.data(); }
    const char* scaleLabel() const { return scale_label_.data(); }
    const char* scaleStatus() const { return scale_status_; }
    const char* controlLabel() const { return control_label_.data(); }
    bool qualitySelected(std::size_t index) const { return selected_[index]; }
    const char* chordLabel() const { return chord_visible_ ? chord_label_.data() : loop_playing_ ? "LOOP" : "READY"; }
    const NoteList& notes() const { return chord_visible_ ? notes_ : empty_; }
    bool chordVisible() const { return chord_visible_; }
    uint8_t brightness() const { return brightness_; }
    bool keyActive() const { return key_active_; }
    PadDisplayState quality(std::size_t index) const { return qualities_[index]; }
    PadDisplayState extension(std::size_t index) const { return extensions_[index]; }

private:
    void formatChord(uint8_t root);
    NoteList notes_{};
    NoteList empty_{};
    std::array<char, 48> key_label_{};
    std::array<char, 40> scale_label_{};
    std::array<char, 48> control_label_{};
    std::array<bool, 4> selected_{};
    const char* scale_status_{"SCALE OFF"};
    std::array<char, 40> chord_label_{};
    std::array<PadDisplayState, 4> qualities_{};
    std::array<PadDisplayState, 4> extensions_{};
    uint64_t last_activity_us_{0};
    uint64_t visible_until_us_{0};
    uint32_t panic_count_{0};
    uint8_t root_{0};
    uint8_t brightness_{0};
    bool chord_visible_{false};
    bool key_active_{false};
    bool loop_playing_{false};
};

}
