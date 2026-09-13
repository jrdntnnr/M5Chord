#pragma once

#include "engine/MusicTypes.h"
#include "engine/PerformanceEngine.h"
#include "midi/MidiRouter.h"

#include <cstdint>

namespace midibrain {

enum class EngineMode : uint8_t {
    Bypass,
    Chord,
    Key
};

enum class BassMode : uint8_t {
    Off,
    Root,
    Lowest,
    Unison
};

enum class PlayStyle : uint8_t { Simple, Advanced, Free, Latched };

struct HarmonicState {
    ChordQuality quality{ChordQuality::Major};
    uint8_t extensions{0};
    bool extension_stack{false};
    uint8_t key_root{0};
    ScaleType scale{ScaleType::Major};
    bool harmonic_quantize{false};
    int8_t voicing_step{0};
    PlayStyle play_style{PlayStyle::Latched};
    bool retrigger_extensions{false};
    int8_t transpose{0};
};

struct BassState {
    BassMode mode{BassMode::Off};
    int8_t octave{-1};
};

struct ClockState {
    uint16_t bpm{100};
    bool output_enabled{false};
    bool running{false};
};

struct RuntimeStats {
    uint32_t midi_events_rx{0};
    uint32_t midi_events_generated{0};
    uint32_t midi_events_tx{0};
    uint32_t midi_events_dropped{0};
    uint32_t panic_count{0};
    uint32_t din_events_rx{0};
    uint32_t din_errors{0};
};

struct AppState {
    EngineMode mode{EngineMode::Bypass};
    HarmonicState harmonic{};
    PerformanceConfig performance{};
    BassState bass{};
    RoutingConfig routing{};
    ClockState clock{};
    RuntimeStats stats{};
    int8_t input_channel{-1};
    uint8_t root_input_low{0};
    uint8_t root_input_high{127};
    uint8_t loop_bars{4};
    uint8_t loop_quantize{0};
    uint8_t preset_slot{0};
    bool velocity_sensitive{true};
    uint8_t input_port{0};
    uint8_t lane_count{4};
};

const char* performanceName(PerformanceMode mode);
const char* modeName(EngineMode mode);

}
