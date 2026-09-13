#pragma once
#include "app/App.h"
#include <cstddef>

namespace midibrain {
struct Parameter { const char* label; SemanticAction action; int minimum; int maximum; bool cycle; };
inline constexpr Parameter parameters[]{
    {"Mode", SemanticAction::ModeNext, 0, 2, true},
    {"Play style", SemanticAction::PlayStyleNext, 0, 3, true},
    {"Extensions", SemanticAction::ExtensionStackToggle, 0, 1, true},
    {"Extension addition", SemanticAction::ExtensionAdditionToggle, 0, 1, true},
    {"Key root", SemanticAction::KeyRootSet, 0, 11, false},
    {"Scale", SemanticAction::ScaleNext, 0, ScaleCount - 1, true},
    {"Transpose", SemanticAction::TransposeSet, -24, 24, false},
    {"Voicing", SemanticAction::VoicingSet, -8, 8, false},
    {"Performance", SemanticAction::PerformanceNext, 0, 7, true},
    {"Direction", SemanticAction::PerformanceDirectionNext, 0, 3, true},
    {"Rate", SemanticAction::PerformanceRateNext, 0, 5, true},
    {"Gate %", SemanticAction::PerformanceGateSet, 1, 100, false},
    {"Strum ms", SemanticAction::StrumIntervalSet, 2, 120, false},
    {"Slop %", SemanticAction::PerformanceAmountSet, 0, 100, false},
    {"BPM", SemanticAction::TempoSet, 30, 300, false},
    {"Clock out", SemanticAction::ClockToggle, 0, 1, true},
    {"Output channel", SemanticAction::PerformanceChannelSet, 1, 16, false},
    {"Bass ch", SemanticAction::BassChannelSet, 1, 16, false},
    {"Raw chord ch", SemanticAction::RawChannelSet, 1, 16, false},
    {"Performance on", SemanticAction::StreamPerformanceToggle, 0, 1, true},
    {"Bass on", SemanticAction::BassToggle, 0, 1, true},
    {"Raw chord on", SemanticAction::StreamRawChordToggle, 0, 1, true},
    {"Bass mode", SemanticAction::BassModeNext, 0, 3, true},
    {"Expression", SemanticAction::ExpressionNext, 0, 2, true},
    {"Input ch (0=all)", SemanticAction::InputChannelSet, 0, 16, false},
    {"Lowest input", SemanticAction::InputLowSet, 0, 127, false},
    {"Highest input", SemanticAction::InputHighSet, 0, 127, false},
    {"Loop bars", SemanticAction::LoopLengthNext, 0, 16, true},
    {"Loop grid", SemanticAction::LoopQuantizeNext, 0, 6, true},
    {"Preset / loop slot", SemanticAction::PresetNext, 1, 16, true},
    {"Harmonic quantize", SemanticAction::HarmonicQuantizeToggle, 0, 1, true},
    {"Bass octave", SemanticAction::BassOctaveUp, -2, 1, true},
    {"Velocity sense", SemanticAction::VelocityToggle, 0, 1, true},
    {"MIDI input", SemanticAction::InputPortNext, 0, 3, true},
    {"Layer channels", SemanticAction::LaneCountSet, 1, 16, false},
};
inline int parameterValue(std::size_t index, const AppState& s) {
    switch (index) {
        case 0: return static_cast<int>(s.mode);
        case 1: return static_cast<int>(s.harmonic.play_style);
        case 2: return s.harmonic.extension_stack;
        case 3: return s.harmonic.retrigger_extensions;
        case 4: return s.harmonic.key_root;
        case 5: return static_cast<int>(s.harmonic.scale);
        case 6: return s.harmonic.transpose;
        case 7: return s.harmonic.voicing_step;
        case 8: return static_cast<int>(s.performance.mode);
        case 9: return static_cast<int>(s.performance.direction);
        case 10: return static_cast<int>(s.performance.division);
        case 11: return s.performance.gate_percent;
        case 12: return s.performance.strum_interval_ms;
        case 13: return s.performance.slop_amount;
        case 14: return s.performance.bpm;
        case 15: return s.clock.output_enabled;
        case 16: return s.routing.performance_channel + 1;
        case 17: return s.routing.bass_channel + 1;
        case 18: return s.routing.raw_chord_channel + 1;
        case 19: return s.routing.performance_enabled;
        case 20: return s.routing.bass_enabled;
        case 21: return s.routing.raw_chord_enabled;
        case 22: return static_cast<int>(s.bass.mode);
        case 23: return static_cast<int>(s.routing.expression_routing);
        case 24: return s.input_channel+1;
        case 25: return s.root_input_low;
        case 26: return s.root_input_high;
        case 27: return s.loop_bars;
        case 28: return s.loop_quantize;
        case 29: return s.preset_slot+1;
        case 30: return s.harmonic.harmonic_quantize;
        case 31: return s.bass.octave;
        case 32: return s.velocity_sensitive;
        case 33: return s.input_port;
        case 34: return s.lane_count;
        default: return 0;
    }
}
inline void adjustParameter(App& app, std::size_t index, int delta, uint64_t nowUs) {
    if (index >= std::size(parameters)) return;
    const auto& parameter = parameters[index];
    if (parameter.cycle) app.apply({parameter.action, static_cast<int16_t>(delta), true}, nowUs);
    else app.apply({parameter.action, static_cast<int16_t>(parameterValue(index, app.state()) + delta), true}, nowUs);
}
void formatParameter(std::size_t index, const AppState& state, char* output, std::size_t size);
}
