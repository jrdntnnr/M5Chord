#pragma once

#include "common/FixedList.h"
#include "midi/MidiEvent.h"
#include "transport/MidiSink.h"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace test {

inline void expect(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + " expected " + expression);
    }
}

class FakeSink final : public midibrain::MidiSink {
public:
    bool send(const midibrain::MidiEvent& event) override { return events.push_back(event); }
    midibrain::FixedList<midibrain::MidiEvent, 2048> events{};
};

}

#define EXPECT(value) ::test::expect((value), #value, __FILE__, __LINE__)

