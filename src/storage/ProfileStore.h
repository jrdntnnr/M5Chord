#pragma once

#include "controller/ControllerMapper.h"
#include "controller/PadLearner.h"
#include "storage/ProfileCodec.h"
#include "app/AppState.h"
#include <array>

namespace midibrain {

class ProfileStore {
public:
    bool begin();
    bool loadActive(ControllerMapper& mapper);
    bool saveSmk37(const FixedList<ControllerMapping, PadLearner::MappingCount>& mappings);
    bool saveActive(const ControllerMapper& mapper, const AppState& state);
    bool next(ControllerMapper& mapper);
    bool resolve(uint16_t vid, uint16_t pid, const char* manufacturer, const char* product, ControllerMapper& mapper);
    void applyInput(AppState& state) const;
    const char* name() const;
    bool available() const;
    const char* error() const;

private:
    void scan();
    bool load(const char* path, ControllerMapper& mapper);
    bool read(const char* path, ControllerProfile& profile);
    ControllerProfile profile_{};
    std::array<std::array<char, 96>, 32> paths_{};
    std::size_t count_{0};
    std::size_t selected_{0};
    char active_path_[96]{"/midi-brain/controllers/smk37.json"};
    bool available_{false};
    const char* error_{"SD absent"};
};

}
