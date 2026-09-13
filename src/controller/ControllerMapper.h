#pragma once

#include "common/FixedList.h"
#include "midi/MidiEvent.h"

#include <cstdint>

namespace midibrain {

enum class SemanticAction : uint8_t {
    None,
    ChordDim,
    ChordMin,
    ChordMaj,
    ChordSus,
    Extension6,
    ExtensionMinor7,
    ExtensionMajor7,
    Extension9,
    ExtensionStackToggle,
    ModeBypass,
    ModeChord,
    ModeKey,
    ModeNext,
    KeyToggle,
    KeyRootSet,
    KeyNext,
    KeyPrev,
    ScaleNext,
    ScalePrev,
    HarmonicQuantizeToggle,
    VoicingUp,
    VoicingDown,
    VoicingDelta,
    VoicingSet,
    PerformanceBlock,
    PerformanceStrum,
    PerformanceStrumTwo,
    PerformanceSlop,
    PerformanceArp,
    PerformanceArpTwo,
    PerformancePattern,
    PerformanceHarp,
    PerformanceNext,
    PerformancePrev,
    PerformanceRateNext,
    PerformanceRatePrev,
    PerformanceGateSet,
    PerformanceAmountSet,
    TempoTap,
    TempoSet,
    TempoUp,
    TempoDown,
    ClockToggle,
    TransportStart,
    TransportStop,
    TransportContinue,
    BassToggle,
    BassModeNext,
    BassOctaveUp,
    BassOctaveDown,
    StreamPerformanceToggle,
    StreamBassToggle,
    StreamRawChordToggle,
    LoopRecord,
    LoopPlay,
    LoopStop,
    LoopOverdub,
    LoopUndo,
    LoopClear,
    PresetNext,
    PresetPrev,
    PresetLoad,
    PresetSave,
    OutputLaneNext,
    Panic,
    ViewNext,
    ViewPrev,
    PlayStyleNext,
    ExtensionAdditionToggle,
    KeyLearn,
    TransposeSet,
    PerformanceDirectionNext,
    StrumIntervalSet,
    PerformanceChannelSet,
    BassChannelSet,
    RawChannelSet,
    ExpressionNext,
    InputChannelSet,
    InputLowSet,
    InputHighSet,
    LoopLengthNext,
    LoopQuantizeNext,
    LoopSave,
    LoopLoad,
    OptionsToggle,
    MenuUp,
    MenuDown,
    MenuDecrease,
    MenuIncrease,
    MenuConfirm,
    MenuBack,
    PadSetup,
    LearnControl,
    MappingDelete,
    ProfileNext,
    ProfileReload,
    ProfileSave,
    DiagnosticsExport,
    MappingEdit,
    VelocityToggle,
    Count
};

enum class MappingTrigger : uint8_t {
    Press,
    Release,
    PressRelease,
    Toggle,
    Value,
    Relative
};

enum class RelativeMode : uint8_t {
    TwosComplement,
    BinaryOffset,
    SignedBit
};

struct SourceSelector {
    MidiType type{MidiType::Unknown};
    int8_t channel{-1};
    int16_t number{-1};
    int16_t minimum{0};
    int16_t maximum{127};
    int8_t cable{-1};
};

struct ControllerMapping {
    SourceSelector source{};
    MappingTrigger trigger{MappingTrigger::Press};
    RelativeMode relative_mode{RelativeMode::TwosComplement};
    SemanticAction action{SemanticAction::None};
    bool consume{false};
    bool toggle_state{false};
    bool down{false};
};

struct ActionEvent {
    SemanticAction action{SemanticAction::None};
    int16_t value{0};
    bool pressed{false};
    uint16_t source{0};
};

struct MappingResult {
    FixedList<ActionEvent, 8> actions{};
    bool consumed{false};
};

class ControllerMapper {
public:
    static constexpr std::size_t MaxMappings = 128;
    bool add(const ControllerMapping& mapping);
    void clear();
    MappingResult map(const MidiEvent& event);
    std::size_t size() const;
    const ControllerMapping& mapping(std::size_t index) const;
    static SemanticAction actionFromName(const char* name);
    static const char* actionName(SemanticAction action);
    bool erase(std::size_t index);
    bool replace(std::size_t index, const ControllerMapping& mapping);
    void resetEdges();

private:
    bool matches(const SourceSelector& selector, const MidiEvent& event) const;
    int16_t relativeValue(uint8_t value, RelativeMode mode) const;
    bool isPress(const MidiEvent& event) const;
    bool isRelease(const MidiEvent& event) const;
    uint8_t eventValue(const MidiEvent& event) const;

    FixedList<ControllerMapping, MaxMappings> mappings_{};
};

}
