#include "storage/SettingsStore.h"
#include "storage/StateCodec.h"

#ifdef ARDUINO
#include <Preferences.h>
#endif

#include <cstddef>
#include <cstdint>

namespace midibrain {

namespace {

struct StoredSettings {
    uint32_t magic{0x4d424d31U};
    uint16_t schema{1};
    uint8_t mode{0};
    uint8_t quality{0};
    uint8_t extensions{0};
    uint8_t key_root{0};
    uint8_t scale{0};
    uint8_t harmonic_quantize{0};
    int8_t voicing_step{0};
    uint8_t performance_mode{0};
    uint8_t direction{0};
    uint8_t division{0};
    uint16_t bpm{100};
    uint8_t gate{75};
    uint16_t strum_interval{25};
    uint8_t slop{20};
    uint8_t bass_mode{0};
    int8_t bass_octave{-1};
    uint8_t performance_channel{0};
    uint8_t bass_channel{1};
    uint8_t raw_channel{2};
    uint8_t stream_flags{1};
    uint8_t expression_routing{1};
    int8_t input_channel{-1};
    uint8_t root_low{0};
    uint8_t root_high{127};
    uint8_t clock_output{0};
    uint32_t checksum{0};
};

[[maybe_unused]] uint32_t hashBytes(const void* data, std::size_t length) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 2166136261U;
    for (std::size_t i = 0; i < length; ++i) {
        hash ^= bytes[i];
        hash *= 16777619U;
    }
    return hash;
}


#ifdef ARDUINO
bool valid(const StoredSettings& stored) {
    return stored.magic == 0x4d424d31U
        && stored.schema == 1
        && stored.checksum == hashBytes(&stored, offsetof(StoredSettings, checksum))
        && stored.mode <= 2
        && stored.quality <= 3
        && (stored.extensions & 0xf0U) == 0
        && stored.key_root < 12
        && stored.scale <= 7
        && stored.voicing_step >= -8 && stored.voicing_step <= 8
        && stored.performance_mode <= 7
        && stored.direction <= 3
        && stored.division <= 5
        && stored.bpm >= 30 && stored.bpm <= 300
        && stored.gate >= 1 && stored.gate <= 100
        && stored.strum_interval >= 2 && stored.strum_interval <= 120
        && stored.slop <= 100
        && stored.bass_mode <= 3
        && stored.bass_octave >= -2 && stored.bass_octave <= 1
        && stored.performance_channel < 16
        && stored.bass_channel < 16
        && stored.raw_channel < 16
        && stored.expression_routing <= 2
        && stored.input_channel >= -1 && stored.input_channel < 16
        && stored.root_low <= stored.root_high;
}
#endif

}

bool SettingsStore::load(AppState& state) {
    state.mode = EngineMode::Chord;
#ifdef ARDUINO
    Preferences preferences;
    if (!preferences.begin("midi-brain", true)) return false;
    const char* key = preferences.isKey("settings-v3") ? "settings-v3" : "settings-v2";
    if (preferences.isKey(key)) {
        char buffer[2048]{};
        const std::size_t length = preferences.getBytesLength(key);
        if (!length || length > sizeof(buffer) || preferences.getBytes(key,buffer,length) != length) {
            preferences.end();
            return false;
        }
        preferences.end();
        JsonDocument document;
        return !deserializeJson(document,buffer,length) && StateCodec::decode(document,state);
    }
    StoredSettings stored;
    const bool read = preferences.getBytesLength("settings") == sizeof(stored) && preferences.getBytes("settings", &stored, sizeof(stored)) == sizeof(stored);
    preferences.end();
    if (!read || !valid(stored)) return false;
    state.mode = static_cast<EngineMode>(stored.mode);
    state.harmonic.quality = static_cast<ChordQuality>(stored.quality);
    state.harmonic.extension_stack = (stored.stream_flags & 16U) != 0;
    state.harmonic.extensions = state.harmonic.extension_stack ? stored.extensions & 0x0fU : 0;
    state.harmonic.key_root = stored.key_root % 12;
    state.harmonic.scale = static_cast<ScaleType>(stored.scale);
    state.harmonic.harmonic_quantize = stored.harmonic_quantize;
    state.harmonic.voicing_step = stored.voicing_step;
    state.performance.mode = static_cast<PerformanceMode>(stored.performance_mode);
    state.performance.direction = static_cast<Direction>(stored.direction);
    state.performance.division = static_cast<Division>(stored.division);
    state.performance.bpm = stored.bpm;
    state.performance.gate_percent = stored.gate;
    state.performance.strum_interval_ms = stored.strum_interval;
    state.performance.slop_amount = stored.slop;
    state.bass.mode = static_cast<BassMode>(stored.bass_mode);
    state.bass.octave = stored.bass_octave;
    state.routing.performance_channel = stored.performance_channel;
    state.routing.bass_channel = stored.bass_channel;
    state.routing.raw_chord_channel = stored.raw_channel;
    state.routing.performance_enabled = (stored.stream_flags & 1U) != 0;
    state.routing.bass_enabled = (stored.stream_flags & 2U) != 0;
    state.routing.raw_chord_enabled = (stored.stream_flags & 4U) != 0;
    state.routing.primary_channel_override = (stored.stream_flags & 8U) != 0;
    state.routing.expression_routing = static_cast<ExpressionRouting>(stored.expression_routing);
    state.input_channel = stored.input_channel;
    state.root_input_low = stored.root_low;
    state.root_input_high = stored.root_high;
    state.clock.output_enabled = stored.clock_output;
    return true;
#else
    static_cast<void>(state);
    return false;
#endif
}

bool SettingsStore::save(const AppState& state) {
#ifdef ARDUINO
    JsonDocument document;
    StateCodec::encode(state,document);
    char buffer[2048]{};
    if (measureJson(document) >= sizeof(buffer)) return false;
    const std::size_t length = serializeJson(document,buffer,sizeof(buffer));
    Preferences preferences;
    if (!preferences.begin("midi-brain", false)) return false;
    const bool written = preferences.putBytes("settings-v3", buffer, length) == length;
    preferences.end();
    return written;
#else
    static_cast<void>(state);
    return false;
#endif
}

uint32_t SettingsStore::fingerprint(const AppState& state) const {
    return StateCodec::fingerprint(state);
}

DisplayView SettingsStore::loadView() const {
#ifdef ARDUINO
    Preferences preferences;
    if (!preferences.begin("midi-brain", true)) return DisplayView::Keyboard;
    const auto value = preferences.getUChar("display-view", static_cast<uint8_t>(DisplayView::Keyboard));
    preferences.end();
    return storedDisplayView(value);
#else
    return DisplayView::Keyboard;
#endif
}
bool SettingsStore::saveView(DisplayView view) {
    const auto value = static_cast<uint8_t>(view);
    if (value >= 4) return false;
#ifdef ARDUINO
    Preferences preferences;
    if (!preferences.begin("midi-brain", false)) return false;
    const bool written = preferences.putUChar("display-view", value) == sizeof(value);
    preferences.end();
    return written;
#else
    return false;
#endif
}

}
