#pragma once

#include "midi/MidiEvent.h"

namespace midibrain {

class MidiSink {
public:
    virtual ~MidiSink() = default;
    virtual bool send(const MidiEvent& event) = 0;
    virtual void discardPending() {}
};

}
