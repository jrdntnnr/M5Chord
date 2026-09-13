#include "controller/ControllerMapper.h"

#include <array>
#include <cstring>

namespace midibrain {

bool ControllerMapper::replace(std::size_t index, const ControllerMapping& mapping) {
    if (index >= mappings_.size() || mapping.action == SemanticAction::None) return false;
    mappings_[index] = mapping;
    mappings_[index].down = false;
    mappings_[index].toggle_state = false;
    return true;
}

namespace {

struct NamedAction {
    const char* name;
    SemanticAction action;
};

constexpr NamedAction namedActions[]{
    {"mapping.edit", SemanticAction::MappingEdit},
    {"velocity.toggle", SemanticAction::VelocityToggle},
    {"chord.dim", SemanticAction::ChordDim},
    {"chord.min", SemanticAction::ChordMin},
    {"chord.maj", SemanticAction::ChordMaj},
    {"chord.sus", SemanticAction::ChordSus},
    {"extension.6", SemanticAction::Extension6},
    {"extension.m7", SemanticAction::ExtensionMinor7},
    {"extension.M7", SemanticAction::ExtensionMajor7},
    {"extension.9", SemanticAction::Extension9},
    {"extension.stack.toggle", SemanticAction::ExtensionStackToggle},
    {"mode.bypass", SemanticAction::ModeBypass},
    {"mode.chord", SemanticAction::ModeChord},
    {"mode.key", SemanticAction::ModeKey},
    {"mode.next", SemanticAction::ModeNext},
    {"key.toggle", SemanticAction::KeyToggle},
    {"key.root.set", SemanticAction::KeyRootSet},
    {"key.next", SemanticAction::KeyNext},
    {"key.prev", SemanticAction::KeyPrev},
    {"scale.next", SemanticAction::ScaleNext},
    {"scale.prev", SemanticAction::ScalePrev},
    {"harmonic_quantize.toggle", SemanticAction::HarmonicQuantizeToggle},
    {"voicing.up", SemanticAction::VoicingUp},
    {"voicing.down", SemanticAction::VoicingDown},
    {"voicing.delta", SemanticAction::VoicingDelta},
    {"voicing.set", SemanticAction::VoicingSet},
    {"performance.block", SemanticAction::PerformanceBlock},
    {"performance.strum", SemanticAction::PerformanceStrum},
    {"performance.strum2", SemanticAction::PerformanceStrumTwo},
    {"performance.slop", SemanticAction::PerformanceSlop},
    {"performance.arp", SemanticAction::PerformanceArp},
    {"performance.arp2", SemanticAction::PerformanceArpTwo},
    {"performance.pattern", SemanticAction::PerformancePattern},
    {"performance.harp", SemanticAction::PerformanceHarp},
    {"performance.next", SemanticAction::PerformanceNext},
    {"performance.prev", SemanticAction::PerformancePrev},
    {"performance.rate.next", SemanticAction::PerformanceRateNext},
    {"performance.rate.prev", SemanticAction::PerformanceRatePrev},
    {"performance.gate.set", SemanticAction::PerformanceGateSet},
    {"performance.amount.set", SemanticAction::PerformanceAmountSet},
    {"tempo.tap", SemanticAction::TempoTap},
    {"tempo.set", SemanticAction::TempoSet},
    {"tempo.up", SemanticAction::TempoUp},
    {"tempo.down", SemanticAction::TempoDown},
    {"clock.toggle", SemanticAction::ClockToggle},
    {"transport.start", SemanticAction::TransportStart},
    {"transport.stop", SemanticAction::TransportStop},
    {"transport.continue", SemanticAction::TransportContinue},
    {"bass.toggle", SemanticAction::BassToggle},
    {"bass.mode.next", SemanticAction::BassModeNext},
    {"bass.octave.up", SemanticAction::BassOctaveUp},
    {"bass.octave.down", SemanticAction::BassOctaveDown},
    {"stream.performance.toggle", SemanticAction::StreamPerformanceToggle},
    {"stream.bass.toggle", SemanticAction::StreamBassToggle},
    {"stream.raw_chord.toggle", SemanticAction::StreamRawChordToggle},
    {"loop.record", SemanticAction::LoopRecord},
    {"loop.play", SemanticAction::LoopPlay},
    {"loop.stop", SemanticAction::LoopStop},
    {"loop.overdub", SemanticAction::LoopOverdub},
    {"loop.undo", SemanticAction::LoopUndo},
    {"loop.clear", SemanticAction::LoopClear},
    {"preset.next", SemanticAction::PresetNext},
    {"preset.prev", SemanticAction::PresetPrev},
    {"preset.load", SemanticAction::PresetLoad},
    {"preset.save", SemanticAction::PresetSave},
    {"output.lane.next", SemanticAction::OutputLaneNext},
    {"panic", SemanticAction::Panic},
    {"view.next", SemanticAction::ViewNext},
    {"view.prev", SemanticAction::ViewPrev},
    {"play_style.next", SemanticAction::PlayStyleNext},
    {"extension.addition.toggle", SemanticAction::ExtensionAdditionToggle},
    {"key.learn", SemanticAction::KeyLearn},
    {"transpose.set", SemanticAction::TransposeSet},
    {"performance.direction.next", SemanticAction::PerformanceDirectionNext},
    {"performance.strum.set", SemanticAction::StrumIntervalSet},
    {"channel.performance.set", SemanticAction::PerformanceChannelSet},
    {"channel.bass.set", SemanticAction::BassChannelSet},
    {"channel.raw.set", SemanticAction::RawChannelSet},
    {"expression.next", SemanticAction::ExpressionNext},
    {"input.channel.set", SemanticAction::InputChannelSet},
    {"input.low.set", SemanticAction::InputLowSet},
    {"input.high.set", SemanticAction::InputHighSet},
    {"loop.length.next", SemanticAction::LoopLengthNext},
    {"loop.quantize.next", SemanticAction::LoopQuantizeNext},
    {"loop.save", SemanticAction::LoopSave},
    {"loop.load", SemanticAction::LoopLoad},
    {"options.toggle", SemanticAction::OptionsToggle},
    {"menu.up", SemanticAction::MenuUp},
    {"menu.down", SemanticAction::MenuDown},
    {"menu.decrease", SemanticAction::MenuDecrease},
    {"menu.increase", SemanticAction::MenuIncrease},
    {"menu.confirm", SemanticAction::MenuConfirm},
    {"menu.back", SemanticAction::MenuBack},
    {"controller.pads", SemanticAction::PadSetup},
    {"controller.learn", SemanticAction::LearnControl},
    {"controller.delete", SemanticAction::MappingDelete},
    {"profile.next", SemanticAction::ProfileNext},
    {"profile.reload", SemanticAction::ProfileReload},
    {"profile.save", SemanticAction::ProfileSave},
    {"diagnostics.export", SemanticAction::DiagnosticsExport},
};

}

bool ControllerMapper::add(const ControllerMapping& mapping) { return mappings_.push_back(mapping); }
void ControllerMapper::clear() { mappings_.clear(); }
void ControllerMapper::resetEdges() { for (auto& mapping : mappings_) mapping.down = false; }
bool ControllerMapper::erase(std::size_t index) {
    if (index >= mappings_.size()) return false;
    mappings_.erase(index);
    return true;
}
std::size_t ControllerMapper::size() const { return mappings_.size(); }
const ControllerMapping& ControllerMapper::mapping(std::size_t index) const { return mappings_[index]; }

bool ControllerMapper::matches(const SourceSelector& selector, const MidiEvent& event) const {
    if (selector.type != event.type && !(selector.type == MidiType::NoteOn && event.type == MidiType::NoteOff)) {
        return false;
    }
    if (selector.channel >= 0 && selector.channel != event.channel) {
        return false;
    }
    if (selector.cable >= 0 && selector.cable != event.cable) {
        return false;
    }
    if (selector.number >= 0) {
        const int number = event.type == MidiType::ProgramChange ? event.data1 : event.data1;
        if (selector.number != number) {
            return false;
        }
    }
    if (isRelease(event)) return true;
    const int value = eventValue(event);
    return value >= selector.minimum && value <= selector.maximum;
}

uint8_t ControllerMapper::eventValue(const MidiEvent& event) const {
    if (event.type == MidiType::PitchBend) {
        return static_cast<uint8_t>(event.value14 >> 7U);
    }
    if (event.type == MidiType::ProgramChange || event.type == MidiType::ChannelPressure) {
        return event.data1;
    }
    return event.data2;
}

bool ControllerMapper::isPress(const MidiEvent& event) const {
    return event.type == MidiType::NoteOn || (event.type == MidiType::ControlChange && event.data2 != 0) || event.type == MidiType::ProgramChange;
}

bool ControllerMapper::isRelease(const MidiEvent& event) const {
    return event.type == MidiType::NoteOff || (event.type == MidiType::ControlChange && event.data2 == 0);
}

int16_t ControllerMapper::relativeValue(uint8_t value, RelativeMode mode) const {
    if (mode == RelativeMode::TwosComplement) {
        return value < 64 ? value : static_cast<int16_t>(value) - 128;
    }
    if (mode == RelativeMode::BinaryOffset) {
        return static_cast<int16_t>(value) - 64;
    }
    if (value == 0 || value == 64) {
        return 0;
    }
    return (value & 0x40U) != 0 ? -static_cast<int16_t>(value & 0x3fU) : value & 0x3fU;
}

MappingResult ControllerMapper::map(const MidiEvent& event) {
    MappingResult result;
    for (auto& mapping : mappings_) {
        if (!matches(mapping.source, event)) {
            continue;
        }
        const bool pressed = isPress(event) && (!mapping.down || event.type == MidiType::ProgramChange);
        const bool released = isRelease(event) && mapping.down;
        if (isPress(event)) mapping.down = true;
        if (released) mapping.down = false;
        bool fire = false;
        int16_t value = eventValue(event);
        switch (mapping.trigger) {
            case MappingTrigger::Press: fire = pressed; break;
            case MappingTrigger::Release: fire = released; break;
            case MappingTrigger::PressRelease: fire = pressed || released; value = pressed ? 1 : 0; break;
            case MappingTrigger::Toggle:
                if (pressed) {
                    mapping.toggle_state = !mapping.toggle_state;
                    value = mapping.toggle_state ? 1 : 0;
                    fire = true;
                }
                break;
            case MappingTrigger::Value: fire = true; break;
            case MappingTrigger::Relative:
                value = relativeValue(eventValue(event), mapping.relative_mode);
                fire = value != 0;
                break;
        }
        if (fire) {
            const auto source = static_cast<uint16_t>(128 + (&mapping - mappings_.begin()));
            const bool harmonic = mapping.action >= SemanticAction::ChordDim && mapping.action <= SemanticAction::Extension9;
            const bool actionPressed = mapping.trigger == MappingTrigger::PressRelease ? pressed
                : mapping.trigger == MappingTrigger::Toggle && harmonic ? value != 0 : true;
            result.actions.push_back({mapping.action, value, actionPressed, source});
        }
        result.consumed = result.consumed || mapping.consume;
    }
    return result;
}

SemanticAction ControllerMapper::actionFromName(const char* name) {
    for (const auto& entry : namedActions) {
        if (std::strcmp(entry.name, name) == 0) {
            return entry.action;
        }
    }
    return SemanticAction::None;
}

const char* ControllerMapper::actionName(SemanticAction action) {
    for (const auto& entry : namedActions) {
        if (entry.action == action) {
            return entry.name;
        }
    }
    return "unknown";
}

}
