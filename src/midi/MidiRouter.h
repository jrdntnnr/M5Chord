#pragma once

#include "common/FixedList.h"
#include "midi/MidiEvent.h"
#include "transport/MidiSink.h"

#include <cstdint>

namespace midibrain {

enum class ExpressionRouting : uint8_t {
    SourceOnly,
    AllActiveGeneratedChannels,
    Disabled
};

struct RoutingConfig {
    uint8_t performance_channel{0};
    uint8_t bass_channel{1};
    uint8_t raw_chord_channel{2};
    bool performance_enabled{true};
    bool bass_enabled{false};
    bool raw_chord_enabled{false};
    bool primary_channel_override{false};
    ExpressionRouting expression_routing{ExpressionRouting::AllActiveGeneratedChannels};
};

class MidiRouter {
public:
    explicit MidiRouter(MidiSink& sink);
    bool forward(const MidiEvent& event);
    void routeExpression(const MidiEvent& event, const RoutingConfig& config);
    void panic();

private:
    MidiSink& sink_;
};

}
