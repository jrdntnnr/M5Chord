#pragma once

#include "app/AppState.h"

#include <cstdint>

namespace midibrain {

class SettingsStore {
public:
    bool load(AppState& state);
    bool save(const AppState& state);
    uint32_t fingerprint(const AppState& state) const;
};

}

