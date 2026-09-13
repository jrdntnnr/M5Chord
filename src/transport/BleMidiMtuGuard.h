#pragma once
#include <cstdint>

namespace midibrain {
class BleMidiMtuGuard {
public:
    bool connected(uint16_t previousConnection, uint16_t connection) {
        if (handled_) return false;
        handled_ = true;
        return previousConnection != connection;
    }
    void reset() { handled_ = false; }
private:
    bool handled_{false};
};
}
