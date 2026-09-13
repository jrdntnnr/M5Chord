#pragma once

namespace midibrain {

class MidiSource {
public:
    virtual ~MidiSource() = default;
    virtual bool begin() = 0;
    virtual void poll() = 0;
    virtual bool connected() const = 0;
};

}

