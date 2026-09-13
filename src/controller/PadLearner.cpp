#include "controller/PadLearner.h"

namespace midibrain {

namespace {

constexpr uint64_t captureGuardUs = 350000;

constexpr SemanticAction actions[PadLearner::MappingCount]{
    SemanticAction::ChordDim,
    SemanticAction::ChordMin,
    SemanticAction::ChordMaj,
    SemanticAction::ChordSus,
    SemanticAction::Extension6,
    SemanticAction::ExtensionMinor7,
    SemanticAction::ExtensionMajor7,
    SemanticAction::Extension9,
    SemanticAction::OutputLaneNext
};

bool eligible(const MidiEvent& event) {
    return event.type == MidiType::NoteOn || event.type == MidiType::ControlChange;
}

int16_t sourceNumber(const MidiEvent& event) {
    return event.type == MidiType::PitchBend || event.type == MidiType::ChannelPressure ? -1 : event.data1;
}

}

void PadLearner::begin() {
    mappings_.clear();
    accept_after_us_ = 0;
    active_ = true;
    awaiting_release_ = false;
}

PadLearnResult PadLearner::capture(const MidiEvent& event) {
    if (awaiting_release_) {
        const bool release = (held_.type == MidiType::NoteOn && (event.type == MidiType::NoteOff || (event.type == MidiType::NoteOn && !event.data2)))
            || (held_.type == MidiType::ControlChange && event.type == MidiType::ControlChange && !event.data2);
        if (release && event.channel == held_.channel && event.data1 == held_.number && event.cable == held_.cable) awaiting_release_ = false;
        return PadLearnResult::Guarded;
    }
    if (!active_ || !eligible(event) || event.data2 == 0) {
        return PadLearnResult::Ignored;
    }
    if (event.timestamp_us < accept_after_us_) return PadLearnResult::Guarded;
    const int16_t number = sourceNumber(event);
    for (const auto& existing : mappings_) {
        if (existing.source.type == event.type && existing.source.channel == event.channel && existing.source.number == number && existing.source.cable == event.cable) {
            return PadLearnResult::Duplicate;
        }
    }
    ControllerMapping mapping;
    mapping.source = {event.type, static_cast<int8_t>(event.channel), number, 0, 127, static_cast<int8_t>(event.cable)};
    mapping.trigger = event.type == MidiType::NoteOn || event.type == MidiType::ControlChange ? MappingTrigger::PressRelease : MappingTrigger::Press;
    mapping.action = actions[mappings_.size()];
    mapping.consume = true;
    mappings_.push_back(mapping);
    held_ = mapping.source;
    awaiting_release_ = true;
    accept_after_us_ = event.timestamp_us + captureGuardUs;
    if (mappings_.size() == MappingCount) {
        active_ = false;
        return PadLearnResult::Complete;
    }
    return PadLearnResult::Captured;
}

bool PadLearner::active() const { return active_; }
uint8_t PadLearner::step() const { return static_cast<uint8_t>(mappings_.size()); }

SemanticAction PadLearner::currentAction() const {
    const std::size_t index = mappings_.size() < MappingCount ? mappings_.size() : MappingCount - 1;
    return actions[index];
}

const FixedList<ControllerMapping, PadLearner::MappingCount>& PadLearner::mappings() const { return mappings_; }

}
