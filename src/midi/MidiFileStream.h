#pragma once
#include "midi/StandardMidiFile.h"
#include <atomic>

namespace midibrain {
class MidiFileStream {
public:
    static constexpr uint32_t Capacity = 512;
    uint32_t request() { return requested_.fetch_add(1, std::memory_order_acq_rel) + 1; }
    uint32_t requested() const { return requested_.load(std::memory_order_acquire); }
    bool ready(uint32_t token) const { return ready_.load(std::memory_order_acquire) == token; }
    void reset() {
        read_.store(0, std::memory_order_relaxed); write_.store(0, std::memory_order_relaxed);
        end_.store(false, std::memory_order_relaxed); error_.store(nullptr, std::memory_order_relaxed);
    }
    void publish(uint32_t token) { ready_.store(token, std::memory_order_release); }
    bool push(const MidiFileEvent& event) {
        const auto write = write_.load(std::memory_order_relaxed);
        if (write - read_.load(std::memory_order_acquire) == Capacity) return false;
        events_[write % Capacity] = event;
        write_.store(write + 1, std::memory_order_release); return true;
    }
    bool peek(MidiFileEvent& event) const {
        const auto read = read_.load(std::memory_order_relaxed);
        if (read == write_.load(std::memory_order_acquire)) return false;
        event = events_[read % Capacity]; return true;
    }
    void pop() { read_.fetch_add(1, std::memory_order_release); }
    uint32_t size() const { return write_.load(std::memory_order_acquire) - read_.load(std::memory_order_acquire); }
    void finish(const char* error = nullptr) { error_.store(error, std::memory_order_release); end_.store(true, std::memory_order_release); }
    bool ended() const { return end_.load(std::memory_order_acquire); }
    const char* error() const { return error_.load(std::memory_order_acquire); }
private:
    std::array<MidiFileEvent, Capacity> events_{};
    std::atomic<uint32_t> requested_{0}, ready_{0}, read_{0}, write_{0};
    std::atomic<bool> end_{false};
    std::atomic<const char*> error_{nullptr};
};
}
