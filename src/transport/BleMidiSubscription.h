#pragma once

#include <cstdint>

namespace midibrain {

class BleMidiSubscription {
public:
    enum class Operation : uint8_t { None, Register, Enable, Disconnect, Read, Encrypt };

    void start(uint64_t nowUs) {
        reset();
        phase_ = Phase::Settle;
        deadline_us_ = nowUs + 500000;
    }

    void reset() { *this = BleMidiSubscription{}; }
    bool ready() const { return ready_; }
    void readComplete(bool success, uint64_t nowUs, bool authenticationRequired = false) {
        if (phase_ != Phase::ReadWait) return;
        phase_ = success ? Phase::Register : authenticationRequired && !security_attempted_ ? Phase::Encrypt : Phase::Failed;
        deadline_us_ = nowUs;
    }
    void encrypted(bool success, uint64_t nowUs) {
        if (phase_ != Phase::EncryptionWait) return;
        phase_ = success ? Phase::Settle : Phase::Failed;
        deadline_us_ = nowUs;
    }

    Operation poll(uint64_t nowUs) {
        if (phase_ == Phase::Idle || nowUs < deadline_us_) return Operation::None;
        switch (phase_) {
            case Phase::Encrypt:
                security_attempted_ = true;
                phase_ = Phase::EncryptionWait;
                deadline_us_ = nowUs + 10000000;
                return Operation::Encrypt;
            case Phase::Settle:
                phase_ = Phase::ReadWait;
                deadline_us_ = nowUs + 2000000;
                return Operation::Read;
            case Phase::Register:
                phase_ = Phase::RegisterWait;
                deadline_us_ = nowUs + 2000000;
                return Operation::Register;
            case Phase::Enable:
                ++attempts_;
                phase_ = Phase::EnableWait;
                deadline_us_ = nowUs + 2000000;
                return Operation::Enable;
            case Phase::RegisterWait:
            case Phase::EnableWait:
            case Phase::ReadWait:
            case Phase::EncryptionWait:
            case Phase::Failed:
                phase_ = Phase::Idle;
                ready_ = false;
                return Operation::Disconnect;
            case Phase::Ready:
                return Operation::None;
            case Phase::Idle:
                return Operation::None;
        }
        return Operation::None;
    }

    void registered(bool success, uint64_t nowUs) {
        if (phase_ != Phase::RegisterWait) return;
        phase_ = success ? Phase::Enable : Phase::Failed;
        deadline_us_ = nowUs;
    }

    void subscribed(bool success, uint64_t nowUs) {
        if (phase_ != Phase::EnableWait) return;
        if (success) {
            ready_ = true;
            phase_ = Phase::Ready;
            deadline_us_ = nowUs + 2000000;
        } else {
            phase_ = attempts_ < 3 ? Phase::Enable : Phase::Failed;
            deadline_us_ = nowUs + 500000;
        }
    }

private:
    enum class Phase : uint8_t { Idle, Settle, ReadWait, Encrypt, EncryptionWait, Register, RegisterWait, Enable, EnableWait, Ready, Failed };
    Phase phase_{Phase::Idle};
    uint64_t deadline_us_{0};
    uint8_t attempts_{0};
    bool ready_{false};
    bool security_attempted_{false};
};

}
