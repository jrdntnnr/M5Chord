#include "storage/SessionStore.h"
#include "storage/StateCodec.h"
#include "storage/LoopCodec.h"
#include "storage/AtomicFile.h"
#include <cstdio>
namespace midibrain {
const char* SessionStore::error() const { return error_; }
bool SessionStore::savePreset(const AppState& state) {
#ifdef ARDUINO
    JsonDocument d;
    StateCodec::encode(state,d);
    char path[80]{};
    std::snprintf(path,sizeof(path),"/midi-brain/presets/preset-%02u.json",state.preset_slot + 1);
    const bool ok = writeJsonFile(path,d);
    error_ = ok ? "PRESET SAVED" : "PRESET SAVE FAILED";
    return ok;
#else
    static_cast<void>(state); return false;
#endif
}
bool SessionStore::loadPreset(AppState& state) {
#ifdef ARDUINO
    JsonDocument d;
    char path[80]{};
    std::snprintf(path,sizeof(path),"/midi-brain/presets/preset-%02u.json",state.preset_slot + 1);
    const bool ok = readJsonFile(path,d,4096) && StateCodec::decode(d,state);
    error_ = ok ? "PRESET LOADED" : "PRESET INVALID / ABSENT";
    return ok;
#else
    static_cast<void>(state); return false;
#endif
}
bool SessionStore::saveLoop(const MidiLooper& loop, uint8_t slot) {
#ifdef ARDUINO
    if (loop.mode() != LoopMode::Stopped) { error_ = "STOP LOOP FIRST"; return false; }
    if (!loop.length() || loop.entries().empty()) { error_ = "LOOP EMPTY"; return false; }
    JsonDocument d;
    LoopCodec::encode(loop,d);
    char path[80]{};
    std::snprintf(path,sizeof(path),"/midi-brain/loops/loop-%02u.json",slot + 1);
    const bool ok = writeJsonFile(path,d);
    error_ = ok ? "LOOP SAVED" : "LOOP SAVE FAILED";
    return ok;
#else
    static_cast<void>(loop); static_cast<void>(slot); return false;
#endif
}
bool SessionStore::loadLoop(MidiLooper& loop, uint8_t slot) {
#ifdef ARDUINO
    JsonDocument d;
    char path[80]{};
    std::snprintf(path,sizeof(path),"/midi-brain/loops/loop-%02u.json",slot + 1);
    error_ = "LOOP INVALID / ABSENT";
    const bool ok = readJsonFile(path,d) && LoopCodec::decode(d,loop);
    if (ok) error_ = "LOOP LOADED";
    return ok;
#else
    static_cast<void>(loop); static_cast<void>(slot); return false;
#endif
}
}
