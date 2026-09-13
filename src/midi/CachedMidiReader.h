#pragma once
#include "midi/StandardMidiFile.h"
#include <algorithm>
#include <cstring>

namespace midibrain {
class CachedMidiReader final : public MidiFileReader {
public:
    explicit CachedMidiReader(MidiFileReader& source) : source_(source) {}
    uint32_t size() const override { return source_.size(); }
    void clear() { for (auto& line : lines_) line.valid = false; clock_ = 0; }
    bool read(uint32_t offset, uint8_t* data, std::size_t count) override {
        if (offset > size() || count > size() - offset) return false;
        while (count) {
            const auto base = offset & ~uint32_t(127);
            Line* found = nullptr;
            for (auto& line : lines_) if (line.valid && line.base == base) { found = &line; break; }
            if (!found) {
                found = &lines_[0];
                for (auto& line : lines_) if (!line.valid || line.used < found->used) { found = &line; if (!line.valid) break; }
                found->valid = false;
                if (!source_.read(base, found->bytes.data(), std::min<uint32_t>(128, size() - base))) return false;
                found->base = base; found->valid = true;
            }
            found->used = ++clock_;
            const auto n = std::min<std::size_t>(count, 128 - (offset - base));
            std::memcpy(data, found->bytes.data() + offset - base, n);
            count -= n; offset += n; data += n;
        }
        return true;
    }
private:
    struct Line { std::array<uint8_t, 128> bytes{}; uint32_t base{0}, used{0}; bool valid{false}; };
    MidiFileReader& source_;
    std::array<Line, 32> lines_{};
    uint32_t clock_{0};
};
}
