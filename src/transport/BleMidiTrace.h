#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace midibrain {

enum class BleTraceEvent : uint8_t {
    Scan, Candidate, Connect, Open, Discovery, Handles, Register, RegisterResult,
    Disable, Enable, WriteResult, Ready, Notification, Disconnect, Error, Reconnect,
    Read, ReadResult, Mtu, SecurityRequest, Authentication, ConnectionParameters, RejectedNotification,
    Link, MtuRequest
};

inline const char* bleTraceName(BleTraceEvent event) {
    constexpr const char* names[]{"SCAN", "CANDIDATE", "CONNECT", "OPEN", "DISCOVERY", "HANDLES", "REGISTER", "REGISTER_RESULT",
        "CCCD_OFF", "CCCD_ON", "WRITE_RESULT", "READY", "NOTIFY", "DISCONNECT", "ERROR", "RECONNECT",
        "READ", "READ_RESULT", "MTU", "SECURITY_REQUEST", "AUTH", "CONN_PARAMS", "NOTIFY_REJECTED",
        "LINK", "MTU_REQUEST"};
    return static_cast<unsigned>(event) < std::size(names) ? names[static_cast<unsigned>(event)] : "UNKNOWN";
}

struct BleTraceRecord {
    uint64_t at_us{0};
    BleTraceEvent event{BleTraceEvent::Scan};
    int32_t status{0};
    uint16_t handle{0};
    uint16_t length{0};
    uint8_t data[16]{};
};

class BleMidiTrace {
public:
    static constexpr std::size_t Capacity = 128;
    void push(const BleTraceRecord& entry) { records_[sequence_++ % Capacity] = entry; }
    uint32_t sequence() const { return sequence_; }
    std::size_t size() const { return sequence_ < Capacity ? sequence_ : Capacity; }
    const BleTraceRecord& at(std::size_t index) const { return records_[(sequence_ - size() + index) % Capacity]; }
private:
    std::array<BleTraceRecord, Capacity> records_{};
    uint32_t sequence_{0};
};

}
