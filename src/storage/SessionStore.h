#pragma once
#include "app/App.h"
namespace midibrain {
class SessionStore {
public:
    bool savePreset(const AppState& state);
    bool loadPreset(AppState& state);
    bool saveLoop(const MidiLooper& loop, uint8_t slot);
    bool loadLoop(MidiLooper& loop, uint8_t slot);
    const char* error() const;
private:
    const char* error_{""};
};
}

