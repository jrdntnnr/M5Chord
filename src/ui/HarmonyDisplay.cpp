#include "ui/HarmonyDisplay.h"

#include <algorithm>
#include <cstdio>

namespace midibrain {
namespace {
constexpr const char* names[]{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
constexpr ChordQuality qualities[]{ChordQuality::Diminished, ChordQuality::Minor, ChordQuality::Major, ChordQuality::Suspended};
constexpr uint8_t extensions[]{Extension6, ExtensionMinor7, ExtensionMajor7, Extension9};
constexpr const char* qualityNames[]{"MAJOR", "MINOR", "DIMINISHED", "SUSPENDED"};
constexpr const char* styleNames[]{"SIMPLE", "ADVANCED", "FREE", "LATCHED"};
}

void HarmonyDisplay::update(const App& app, uint64_t nowUs) {
    const auto& state = app.state();
    const bool bypass = state.mode == EngineMode::Bypass;
    const bool automatic = state.mode == EngineMode::Key && !app.heldQualities();
    const bool override = state.mode == EngineMode::Key && app.heldQualities();
    const bool latched = state.harmonic.play_style == PlayStyle::Latched;
    key_active_ = automatic || (!bypass && state.harmonic.harmonic_quantize);
    scale_status_ = bypass ? "SCALE OFF" : automatic ? "AUTO SCALE" : key_active_ ? "SNAP ON" : override ? "OVERRIDE" : "SNAP OFF";
    std::snprintf(scale_label_.data(), scale_label_.size(), "%s %s", names[state.harmonic.key_root], ScaleEngine{}.definition(state.harmonic.scale).name);
    std::snprintf(key_label_.data(), key_label_.size(), "KEY: %s %s%s", names[state.harmonic.key_root],
        ScaleEngine{}.definition(state.harmonic.scale).name, key_active_ ? "" : " (OFF)");
    for (std::size_t i = 0; i < qualities_.size(); ++i) {
        selected_[i] = !bypass && state.harmonic.quality == qualities[i] && !automatic && (latched || app.heldQualities());
        qualities_[i] = bypass ? PadDisplayState::Disabled : app.heldQualities() & (1U << i) ? PadDisplayState::Held : selected_[i] ? PadDisplayState::Selected : PadDisplayState::Off;
        extensions_[i] = bypass ? PadDisplayState::Disabled : !(state.harmonic.extensions & extensions[i]) ? PadDisplayState::Off
            : state.harmonic.extension_stack ? PadDisplayState::Selected : PadDisplayState::Held;
    }
    if (bypass) {
        std::snprintf(control_label_.data(), control_label_.size(), "DIRECT MIDI / MODIFIERS OFF");
    } else if (automatic) {
        std::snprintf(control_label_.data(), control_label_.size(), "AUTO CHORD / %s", styleNames[static_cast<unsigned>(state.harmonic.play_style)]);
    } else {
        const bool chosen = latched || app.heldQualities();
        std::snprintf(control_label_.data(), control_label_.size(), "%s / %s", chosen ? qualityNames[static_cast<unsigned>(state.harmonic.quality)] : "NO TYPE HELD", styleNames[static_cast<unsigned>(state.harmonic.play_style)]);
    }
    const bool panic = panic_count_ != state.stats.panic_count;
    panic_count_ = state.stats.panic_count;
    const bool held = app.hasHeldRoots();
    const bool activity = app.lastActivity() != last_activity_us_;
    last_activity_us_ = app.lastActivity();
    loop_playing_ = app.looper().mode() == LoopMode::Playing || app.looper().mode() == LoopMode::Overdub;
    if (panic) visible_until_us_ = 0;
    if (!app.lastChord().empty() && (held || (activity && !panic))) {
        const uint8_t root = static_cast<uint8_t>((app.lastRoot() + (state.mode == EngineMode::Bypass ? 0 : state.harmonic.transpose) + 120) % 12);
        bool changed = notes_.size() != app.lastChord().size() || root_ != root;
        if (!changed) for (std::size_t i = 0; i < notes_.size(); ++i) changed = changed || notes_[i] != app.lastChord()[i];
        if (changed) {
            notes_ = app.lastChord();
            root_ = root;
            formatChord(root);
        }
        visible_until_us_ = (held ? nowUs : app.lastActivity()) + LingerUs;
    }
    chord_visible_ = !notes_.empty() && visible_until_us_ && nowUs < visible_until_us_;
    const uint64_t remaining = chord_visible_ ? visible_until_us_ - nowUs : 0;
    brightness_ = static_cast<uint8_t>(std::min<uint64_t>(255, remaining * 255 / FadeUs));
}

void HarmonyDisplay::formatChord(uint8_t root) {
    if (notes_.size() == 1) {
        const auto note = notes_[0];
        std::snprintf(chord_label_.data(), chord_label_.size(), "%s%d", names[note % 12], note / 12 - 1);
        return;
    }
    uint16_t actual = 0;
    for (const auto note : notes_) actual |= 1U << (note % 12);
    for (unsigned offset = 0; offset < 12; ++offset) {
        const uint8_t candidate = (root + offset) % 12;
        for (uint8_t extension = 0; extension < 16; ++extension) {
            for (uint8_t quality = 0; quality < 4; ++quality) {
                uint16_t mask = 0;
                for (const auto note : ChordEngine{}.build(candidate + 48, static_cast<ChordQuality>(quality), extension)) mask |= 1U << (note % 12);
                if (mask != actual) continue;
                ChordEngine{}.formatName(chord_label_.data(), chord_label_.size(), candidate, static_cast<ChordQuality>(quality), extension);
                return;
            }
        }
    }
    std::snprintf(chord_label_.data(), chord_label_.size(), "%s [%u NOTES]", names[root], static_cast<unsigned>(notes_.size()));
}

}
