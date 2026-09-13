#pragma once

#include "app/AppState.h"
#include "common/DisplayView.h"

#include <cstdint>

namespace midibrain {

class SettingsStore {
public:
    bool load(AppState& state);
    bool save(const AppState& state);
    DisplayView loadView() const;
    bool saveView(DisplayView view);
    uint32_t fingerprint(const AppState& state) const;
};

}
