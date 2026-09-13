#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace midibrain {
struct DiagnosticSample {
    uint64_t at_us{0};
    uint32_t rx{0};
    uint32_t tx{0};
    uint32_t dropped{0};
    uint32_t late_us{0};
    uint32_t ble_dropped{0};
    uint32_t din_dropped{0};
    uint16_t active{0};
    uint16_t queued{0};
    uint8_t loop_mode{0};
};
class DiagnosticRing {
public:
    static constexpr std::size_t Capacity = 64;
    void push(const DiagnosticSample& sample) {
        entries_[next_] = sample;
        next_ = (next_ + 1) % Capacity;
        if (size_ < Capacity) ++size_;
    }
    std::size_t size() const { return size_; }
    const DiagnosticSample& at(std::size_t index) const { return entries_[(next_ + Capacity - size_ + index) % Capacity]; }
private:
    std::array<DiagnosticSample,Capacity> entries_{};
    std::size_t next_{0};
    std::size_t size_{0};
};
}
