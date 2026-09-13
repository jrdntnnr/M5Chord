#pragma once
#include "midi/MidiEvent.h"

namespace midibrain {

enum class InputPort : uint8_t { Auto, Ble, Usb, Din };

class InputSelection {
public:
    void configure(InputPort port) { configured_ = port; active_ = port; }
    bool accept(InputPort port, const MidiEvent& event) {
        if (port == InputPort::Auto || event.type == MidiType::Unknown || event.type == MidiType::SysEx) return false;
        if (active_ == InputPort::Auto) {
            if (event.type > MidiType::PitchBend) return false;
            active_ = port;
        }
        return active_ == port;
    }
    bool disconnected(InputPort port) {
        if (active_ != port) return false;
        active_ = configured_;
        return true;
    }
    InputPort active() const { return active_; }
private:
    InputPort configured_{InputPort::Auto};
    InputPort active_{InputPort::Auto};
};

}
