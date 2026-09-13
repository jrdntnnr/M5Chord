#include "midi/MidiRouter.h"

#include <array>

namespace midibrain {

MidiRouter::MidiRouter(MidiSink& sink) : sink_(sink) {}

bool MidiRouter::forward(const MidiEvent& event) { return sink_.send(event); }

void MidiRouter::routeExpression(const MidiEvent& event, const RoutingConfig& config) {
    if (config.expression_routing == ExpressionRouting::Disabled) {
        return;
    }
    if (config.expression_routing == ExpressionRouting::SourceOnly) {
        sink_.send(event);
        return;
    }
    std::array<bool, 16> channels{};
    if (config.performance_enabled) channels[config.performance_channel] = true;
    if (config.bass_enabled) channels[config.bass_channel] = true;
    if (config.raw_chord_enabled) channels[config.raw_chord_channel] = true;
    for (uint8_t channel = 0; channel < channels.size(); ++channel) {
        if (channels[channel]) {
            MidiEvent routed = event;
            routed.channel = channel;
            sink_.send(routed);
        }
    }
}

void MidiRouter::panic() {
    for (uint8_t channel = 0; channel < 16; ++channel) {
        sink_.send(MidiEvent::cc(channel, 64, 0));
        sink_.send(MidiEvent::cc(channel, 120, 0));
        sink_.send(MidiEvent::cc(channel, 121, 0));
        sink_.send(MidiEvent::cc(channel, 123, 0));
    }
}

}
