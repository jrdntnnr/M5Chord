#include "storage/StateCodec.h"
namespace midibrain {
namespace {
uint32_t stateFingerprint(const AppState& state, bool current) {
    const int values[]{
        static_cast<int>(state.mode),
        static_cast<int>(state.harmonic.quality),
        state.harmonic.extension_stack ? state.harmonic.extensions : 0,
        static_cast<int>(state.harmonic.extension_stack),
        static_cast<int>(state.harmonic.key_root),
        static_cast<int>(state.harmonic.scale),
        static_cast<int>(state.harmonic.harmonic_quantize),
        static_cast<int>(state.harmonic.voicing_step),
        static_cast<int>(state.harmonic.play_style),
        static_cast<int>(state.harmonic.retrigger_extensions),
        static_cast<int>(state.harmonic.transpose),
        static_cast<int>(state.performance.mode),
        static_cast<int>(state.performance.direction),
        static_cast<int>(state.performance.division),
        static_cast<int>(state.performance.bpm),
        static_cast<int>(state.performance.gate_percent),
        static_cast<int>(state.performance.strum_interval_ms),
        static_cast<int>(state.performance.slop_amount),
        static_cast<int>(state.bass.mode),
        static_cast<int>(state.bass.octave),
        static_cast<int>(state.routing.performance_channel),
        static_cast<int>(state.routing.bass_channel),
        static_cast<int>(state.routing.raw_chord_channel),
        static_cast<int>(state.routing.performance_enabled),
        static_cast<int>(state.routing.bass_enabled),
        static_cast<int>(state.routing.raw_chord_enabled),
        static_cast<int>(state.routing.primary_channel_override),
        static_cast<int>(state.routing.expression_routing),
        static_cast<int>(state.input_channel),
        static_cast<int>(state.root_input_low),
        static_cast<int>(state.root_input_high),
        static_cast<int>(state.clock.output_enabled),
        static_cast<int>(state.loop_bars),
        static_cast<int>(state.loop_quantize),
        static_cast<int>(state.preset_slot),
        static_cast<int>(state.velocity_sensitive),
    };
    uint32_t hash = 2166136261U;
    for (int value : values) {
        for (unsigned shift = 0; shift < 32; shift += 8) { hash ^= (static_cast<uint32_t>(value) >> shift) & 255U; hash *= 16777619U; }
    }
    if (current) for (int value : {static_cast<int>(state.input_port), static_cast<int>(state.lane_count)}) {
        for (unsigned shift = 0; shift < 32; shift += 8) { hash ^= (static_cast<uint32_t>(value) >> shift) & 255U; hash *= 16777619U; }
    }
    return hash;
}
}
uint32_t StateCodec::fingerprint(const AppState& state) { return stateFingerprint(state, true); }
void StateCodec::encode(const AppState& state, JsonDocument& document) {
    document.clear();
    document["schema"] = 3;
    document["input_port"] = state.input_port;
    document["lane_count"] = state.lane_count;
    document["mode"] = static_cast<int>(state.mode);
    document["quality"] = static_cast<int>(state.harmonic.quality);
    document["extensions"] = state.harmonic.extension_stack ? state.harmonic.extensions : 0;
    document["stack"] = static_cast<int>(state.harmonic.extension_stack);
    document["key"] = static_cast<int>(state.harmonic.key_root);
    document["scale"] = static_cast<int>(state.harmonic.scale);
    document["quantize"] = static_cast<int>(state.harmonic.harmonic_quantize);
    document["voicing"] = static_cast<int>(state.harmonic.voicing_step);
    document["style"] = static_cast<int>(state.harmonic.play_style);
    document["retrigger"] = static_cast<int>(state.harmonic.retrigger_extensions);
    document["transpose"] = static_cast<int>(state.harmonic.transpose);
    document["performance"] = static_cast<int>(state.performance.mode);
    document["direction"] = static_cast<int>(state.performance.direction);
    document["division"] = static_cast<int>(state.performance.division);
    document["bpm"] = static_cast<int>(state.performance.bpm);
    document["gate"] = static_cast<int>(state.performance.gate_percent);
    document["strum"] = static_cast<int>(state.performance.strum_interval_ms);
    document["slop"] = static_cast<int>(state.performance.slop_amount);
    document["bass_mode"] = static_cast<int>(state.bass.mode);
    document["bass_octave"] = static_cast<int>(state.bass.octave);
    document["performance_ch"] = static_cast<int>(state.routing.performance_channel);
    document["bass_ch"] = static_cast<int>(state.routing.bass_channel);
    document["raw_ch"] = static_cast<int>(state.routing.raw_chord_channel);
    document["performance_on"] = static_cast<int>(state.routing.performance_enabled);
    document["bass_on"] = static_cast<int>(state.routing.bass_enabled);
    document["raw_on"] = static_cast<int>(state.routing.raw_chord_enabled);
    document["channel_override"] = static_cast<int>(state.routing.primary_channel_override);
    document["expression"] = static_cast<int>(state.routing.expression_routing);
    document["input"] = static_cast<int>(state.input_channel);
    document["low"] = static_cast<int>(state.root_input_low);
    document["high"] = static_cast<int>(state.root_input_high);
    document["clock"] = static_cast<int>(state.clock.output_enabled);
    document["loop_bars"] = static_cast<int>(state.loop_bars);
    document["loop_grid"] = static_cast<int>(state.loop_quantize);
    document["slot"] = static_cast<int>(state.preset_slot);
    document["velocity"] = static_cast<int>(state.velocity_sensitive);
    document["checksum"] = fingerprint(state);
}
bool StateCodec::decode(const JsonDocument& document, AppState& state) {
    if (document["schema"] != 2 && document["schema"] != 3) return false;
    AppState decoded;
    if (!document["mode"].is<int>() || document["mode"].as<int>() < 0 || document["mode"].as<int>() > 2) return false;
    decoded.mode = static_cast<EngineMode>(document["mode"].as<int>());
    if (!document["quality"].is<int>() || document["quality"].as<int>() < 0 || document["quality"].as<int>() > 3) return false;
    decoded.harmonic.quality = static_cast<ChordQuality>(document["quality"].as<int>());
    if (!document["extensions"].is<int>() || document["extensions"].as<int>() < 0 || document["extensions"].as<int>() > 15) return false;
    decoded.harmonic.extensions = static_cast<uint8_t>(document["extensions"].as<int>());
    if (!document["stack"].is<int>() || document["stack"].as<int>() < 0 || document["stack"].as<int>() > 1) return false;
    decoded.harmonic.extension_stack = static_cast<bool>(document["stack"].as<int>());
    if (!document["key"].is<int>() || document["key"].as<int>() < 0 || document["key"].as<int>() > 11) return false;
    decoded.harmonic.key_root = static_cast<uint8_t>(document["key"].as<int>());
    if (!document["scale"].is<int>() || document["scale"].as<int>() < 0 || document["scale"].as<int>() >= ScaleCount) return false;
    decoded.harmonic.scale = static_cast<ScaleType>(document["scale"].as<int>());
    if (!document["quantize"].is<int>() || document["quantize"].as<int>() < 0 || document["quantize"].as<int>() > 1) return false;
    decoded.harmonic.harmonic_quantize = static_cast<bool>(document["quantize"].as<int>());
    if (!document["voicing"].is<int>() || document["voicing"].as<int>() < -8 || document["voicing"].as<int>() > 8) return false;
    decoded.harmonic.voicing_step = static_cast<int8_t>(document["voicing"].as<int>());
    if (!document["style"].is<int>() || document["style"].as<int>() < 0 || document["style"].as<int>() > 3) return false;
    decoded.harmonic.play_style = static_cast<PlayStyle>(document["style"].as<int>());
    if (!document["retrigger"].is<int>() || document["retrigger"].as<int>() < 0 || document["retrigger"].as<int>() > 1) return false;
    decoded.harmonic.retrigger_extensions = static_cast<bool>(document["retrigger"].as<int>());
    if (!document["transpose"].is<int>() || document["transpose"].as<int>() < -24 || document["transpose"].as<int>() > 24) return false;
    decoded.harmonic.transpose = static_cast<int8_t>(document["transpose"].as<int>());
    if (!document["performance"].is<int>() || document["performance"].as<int>() < 0 || document["performance"].as<int>() > 7) return false;
    decoded.performance.mode = static_cast<PerformanceMode>(document["performance"].as<int>());
    if (!document["direction"].is<int>() || document["direction"].as<int>() < 0 || document["direction"].as<int>() > 3) return false;
    decoded.performance.direction = static_cast<Direction>(document["direction"].as<int>());
    if (!document["division"].is<int>() || document["division"].as<int>() < 0 || document["division"].as<int>() > 5) return false;
    decoded.performance.division = static_cast<Division>(document["division"].as<int>());
    if (!document["bpm"].is<int>() || document["bpm"].as<int>() < 30 || document["bpm"].as<int>() > 300) return false;
    decoded.performance.bpm = static_cast<uint16_t>(document["bpm"].as<int>());
    if (!document["gate"].is<int>() || document["gate"].as<int>() < 1 || document["gate"].as<int>() > 100) return false;
    decoded.performance.gate_percent = static_cast<uint8_t>(document["gate"].as<int>());
    if (!document["strum"].is<int>() || document["strum"].as<int>() < 2 || document["strum"].as<int>() > 120) return false;
    decoded.performance.strum_interval_ms = static_cast<uint16_t>(document["strum"].as<int>());
    if (!document["slop"].is<int>() || document["slop"].as<int>() < 0 || document["slop"].as<int>() > 100) return false;
    decoded.performance.slop_amount = static_cast<uint8_t>(document["slop"].as<int>());
    if (!document["bass_mode"].is<int>() || document["bass_mode"].as<int>() < 0 || document["bass_mode"].as<int>() > 3) return false;
    decoded.bass.mode = static_cast<BassMode>(document["bass_mode"].as<int>());
    if (!document["bass_octave"].is<int>() || document["bass_octave"].as<int>() < -2 || document["bass_octave"].as<int>() > 1) return false;
    decoded.bass.octave = static_cast<int8_t>(document["bass_octave"].as<int>());
    if (!document["performance_ch"].is<int>() || document["performance_ch"].as<int>() < 0 || document["performance_ch"].as<int>() > 15) return false;
    decoded.routing.performance_channel = static_cast<uint8_t>(document["performance_ch"].as<int>());
    if (!document["bass_ch"].is<int>() || document["bass_ch"].as<int>() < 0 || document["bass_ch"].as<int>() > 15) return false;
    decoded.routing.bass_channel = static_cast<uint8_t>(document["bass_ch"].as<int>());
    if (!document["raw_ch"].is<int>() || document["raw_ch"].as<int>() < 0 || document["raw_ch"].as<int>() > 15) return false;
    decoded.routing.raw_chord_channel = static_cast<uint8_t>(document["raw_ch"].as<int>());
    if (!document["performance_on"].is<int>() || document["performance_on"].as<int>() < 0 || document["performance_on"].as<int>() > 1) return false;
    decoded.routing.performance_enabled = static_cast<bool>(document["performance_on"].as<int>());
    if (!document["bass_on"].is<int>() || document["bass_on"].as<int>() < 0 || document["bass_on"].as<int>() > 1) return false;
    decoded.routing.bass_enabled = static_cast<bool>(document["bass_on"].as<int>());
    if (!document["raw_on"].is<int>() || document["raw_on"].as<int>() < 0 || document["raw_on"].as<int>() > 1) return false;
    decoded.routing.raw_chord_enabled = static_cast<bool>(document["raw_on"].as<int>());
    if (!document["channel_override"].is<int>() || document["channel_override"].as<int>() < 0 || document["channel_override"].as<int>() > 1) return false;
    decoded.routing.primary_channel_override = static_cast<bool>(document["channel_override"].as<int>());
    if (!document["expression"].is<int>() || document["expression"].as<int>() < 0 || document["expression"].as<int>() > 2) return false;
    decoded.routing.expression_routing = static_cast<ExpressionRouting>(document["expression"].as<int>());
    if (!document["input"].is<int>() || document["input"].as<int>() < -1 || document["input"].as<int>() > 15) return false;
    decoded.input_channel = static_cast<int8_t>(document["input"].as<int>());
    if (!document["low"].is<int>() || document["low"].as<int>() < 0 || document["low"].as<int>() > 127) return false;
    decoded.root_input_low = static_cast<uint8_t>(document["low"].as<int>());
    if (!document["high"].is<int>() || document["high"].as<int>() < 0 || document["high"].as<int>() > 127) return false;
    decoded.root_input_high = static_cast<uint8_t>(document["high"].as<int>());
    if (!document["clock"].is<int>() || document["clock"].as<int>() < 0 || document["clock"].as<int>() > 1) return false;
    decoded.clock.output_enabled = static_cast<bool>(document["clock"].as<int>());
    if (!document["loop_bars"].is<int>() || document["loop_bars"].as<int>() < 0 || document["loop_bars"].as<int>() > 16) return false;
    decoded.loop_bars = static_cast<uint8_t>(document["loop_bars"].as<int>());
    if (!document["loop_grid"].is<int>() || document["loop_grid"].as<int>() < 0 || document["loop_grid"].as<int>() > 6) return false;
    decoded.loop_quantize = static_cast<uint8_t>(document["loop_grid"].as<int>());
    if (!document["slot"].is<int>() || document["slot"].as<int>() < 0 || document["slot"].as<int>() > 15) return false;
    decoded.preset_slot = static_cast<uint8_t>(document["slot"].as<int>());
    if (!document["velocity"].is<int>() || document["velocity"].as<int>() < 0 || document["velocity"].as<int>() > 1) return false;
    decoded.velocity_sensitive = document["velocity"].as<int>() != 0;
    if (decoded.root_input_low > decoded.root_input_high) return false;
    if (decoded.loop_bars && (decoded.loop_bars & (decoded.loop_bars - 1))) return false;
    if (document["schema"] == 3) {
        if (!document["input_port"].is<int>() || document["input_port"].as<int>() < 0 || document["input_port"].as<int>() > 3) return false;
        if (!document["lane_count"].is<int>() || document["lane_count"].as<int>() < 1 || document["lane_count"].as<int>() > 16) return false;
        decoded.input_port = document["input_port"].as<uint8_t>();
        decoded.lane_count = document["lane_count"].as<uint8_t>();
    }
    if (!document["checksum"].is<uint32_t>() || document["checksum"].as<uint32_t>() != stateFingerprint(decoded, document["schema"] == 3)) return false;
    if (!decoded.harmonic.extension_stack) decoded.harmonic.extensions = 0;
    decoded.stats = state.stats;
    state = decoded;
    return true;
}
}
