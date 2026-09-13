#pragma once
#include "midi/StandardMidiFile.h"
#include "midi/MidiFileStream.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace midibrain {
struct MidiFileCatalog {
    FixedList<std::array<char, 64>, 32> names{};
    bool truncated{false};
    bool add(const char* name) {
        if (!name || name[0] == '.' || std::strchr(name, '/') || std::strchr(name, '\\')) return false;
        const auto length = std::strlen(name);
        if (length < 5 || length >= 64) { truncated = true; return false; }
        const char* extension = std::strrchr(name, '.');
        if (!extension) return false;
        char lower[6]{};
        if (std::strlen(extension) >= sizeof(lower)) return false;
        for (unsigned i = 0; extension[i]; ++i) lower[i] = extension[i] >= 'A' && extension[i] <= 'Z' ? extension[i] + 32 : extension[i];
        if (std::strcmp(lower, ".mid") && std::strcmp(lower, ".midi")) return false;
        for (const auto& item : names) if (!std::strcmp(item.data(), name)) return false;
        std::array<char, 64> item{};
        std::memcpy(item.data(), name, length);
        if (!names.push_back(item)) { truncated = true; return false; }
        std::sort(names.begin(), names.end(), [](const auto& a, const auto& b) { return std::strcmp(a.data(), b.data()) < 0; });
        return true;
    }
};
class MidiFileStore {
public:
    bool scan(MidiFileCatalog& catalog);
    bool beginLoad(const char* name);
    bool complete(StandardMidiFile& file);
    bool loading() const { return loading_; }
    uint8_t progress() const { return progress_.load(std::memory_order_relaxed); }
    MidiFileStream& stream() { return stream_; }
    const char* status() const { return status_; }
private:
    struct Worker;
    Worker* worker_{nullptr};
    MidiFileStream stream_{};
    std::atomic<bool> load_pending_{false}, complete_{false};
    std::atomic<uint8_t> progress_{0};
    bool loading_{false};
    char pending_name_[64]{};
    uint64_t duration_{0};
    uint16_t channels_{0}, tracks_{0};
    uint32_t skipped_sysex_{0};
    const char* result_{nullptr};
    const char* status_{"NO SD CARD"};
};
}
