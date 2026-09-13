#include "storage/ProfileStore.h"
#include "storage/AtomicFile.h"
#include "hardware/CardputerHardware.h"
#include <cstdio>
#include <cstring>
#ifdef ARDUINO
#include <Preferences.h>
#include <SPI.h>
#endif

namespace midibrain {
bool ProfileStore::begin() {
#ifdef ARDUINO
    SPI.begin(CardputerPins::SdClock, CardputerPins::SdMiso, CardputerPins::SdMosi, CardputerPins::SdSelect);
    available_ = SD.begin(CardputerPins::SdSelect,SPI,25000000);
    if (available_) {
        SD.mkdir("/midi-brain");
        SD.mkdir("/midi-brain/controllers");
        SD.mkdir("/midi-brain/presets");
        SD.mkdir("/midi-brain/loops");
        SD.mkdir("/midi-brain/logs");
        Preferences preferences;
        if (preferences.begin("midi-brain",true)) {
            if (preferences.isKey("profile")) preferences.getString("profile",active_path_,sizeof(active_path_));
            preferences.end();
            if (std::strncmp(active_path_,"/midi-brain/controllers/",23) || std::strstr(active_path_,".."))
                std::snprintf(active_path_,sizeof(active_path_),"/midi-brain/controllers/smk37.json");
        }
        scan();
    }
#endif
    error_ = available_ ? "" : "SD mount failed";
    return available_;
}
void ProfileStore::scan() {
    count_ = 0;
#ifdef ARDUINO
    File directory = SD.open("/midi-brain/controllers");
    if (!directory) return;
    for (File file = directory.openNextFile(); file && count_ < paths_.size(); file = directory.openNextFile()) {
        const char* name = file.name();
        const std::size_t size = std::strlen(name);
        if (!file.isDirectory() && size > 5 && !std::strcmp(name + size - 5,".json")) {
            const char* base = std::strrchr(name,'/');
            base = base ? base + 1 : name;
            const int written = std::snprintf(paths_[count_].data(),paths_[count_].size(),"/midi-brain/controllers/%s",base);
            if (written > 0 && written < static_cast<int>(paths_[count_].size())) ++count_;
        }
        file.close();
    }
    directory.close();
#endif
}
bool ProfileStore::read(const char* path, ControllerProfile& profile) {
#ifdef ARDUINO
    JsonDocument document;
    if (!readJsonFile(path,document)) { error_ = "Profile read failed"; return false; }
    return ProfileCodec::decode(document,profile,error_);
#else
    static_cast<void>(path); static_cast<void>(profile); return false;
#endif
}
bool ProfileStore::load(const char* path, ControllerMapper& mapper) {
    if (!available_ || !read(path,profile_)) {
        mapper.clear();
        profile_ = ControllerProfile{};
        return false;
    }
    mapper = profile_.mapper;
    std::snprintf(active_path_,sizeof(active_path_),"%s",path);
    error_ = "";
    return true;
}
bool ProfileStore::loadActive(ControllerMapper& mapper) {
    char path[96]{};
    std::snprintf(path,sizeof(path),"%s",active_path_);
    return load(path,mapper);
}
bool ProfileStore::saveActive(const ControllerMapper& mapper, const AppState& state) {
    if (!available_) { error_ = "SD absent"; return false; }
    profile_.mapper = mapper;
    profile_.input_channel = state.input_channel;
    profile_.low = state.root_input_low;
    profile_.high = state.root_input_high;
#ifdef ARDUINO
    JsonDocument document;
    ProfileCodec::encode(profile_,document);
    if (!writeJsonFile(active_path_,document)) { error_ = "Profile save failed"; return false; }
    scan();
    error_ = "";
    return true;
#else
    return false;
#endif
}
bool ProfileStore::saveSmk37(const FixedList<ControllerMapping,PadLearner::MappingCount>& mappings) {
    if (mappings.size() != PadLearner::MappingCount) return false;
    profile_ = ControllerProfile{};
    std::snprintf(profile_.name,sizeof(profile_.name),"SMK37");
    std::snprintf(active_path_,sizeof(active_path_),"/midi-brain/controllers/smk37.json");
    for (const auto& m : mappings) profile_.mapper.add(m);
    const AppState state{};
    return saveActive(profile_.mapper,state);
}
bool ProfileStore::next(ControllerMapper& mapper) {
    scan();
    if (!count_) { error_ = "No SD profiles"; return false; }
    for (std::size_t i = 0; i < count_; ++i) if (!std::strcmp(paths_[i].data(),active_path_)) selected_ = i;
    selected_ = (selected_ + 1) % count_;
    const bool loaded = load(paths_[selected_].data(),mapper);
#ifdef ARDUINO
    if (loaded) {
        Preferences preferences;
        if (preferences.begin("midi-brain",false)) {
            preferences.putString("profile",active_path_);
            preferences.end();
        }
    }
#endif
    return loaded;
}
bool ProfileStore::resolve(uint16_t vid, uint16_t pid, const char* manufacturer, const char* product, ControllerMapper& mapper) {
    if (!available_) return false;
    scan();
    int score = 0;
    std::size_t best = count_;
    for (std::size_t i = 0; i < count_; ++i) {
        if (!read(paths_[i].data(),profile_)) continue;
        const int current = ProfileCodec::match(profile_,vid,pid,manufacturer,product);
        if (current > score) { score = current; best = i; }
    }
    if (best < count_) { selected_ = best; return load(paths_[best].data(),mapper); }
    return loadActive(mapper);
}
void ProfileStore::applyInput(AppState& state) const {
    state.input_channel = profile_.input_channel;
    state.root_input_low = profile_.low;
    state.root_input_high = profile_.high;
}
bool ProfileStore::available() const { return available_; }
const char* ProfileStore::error() const { return error_; }
const char* ProfileStore::name() const { return profile_.name; }
}
