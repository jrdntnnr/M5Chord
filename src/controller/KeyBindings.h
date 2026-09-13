#pragma once
#include "controller/ControllerMapper.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace midibrain {

struct KeyBinding { uint8_t key; uint8_t modifiers; SemanticAction action; bool release; bool menu; };
inline constexpr KeyBinding keyBindings[]{
    {113, 0, SemanticAction::ChordDim, true, false},
    {119, 0, SemanticAction::ChordMin, true, false},
    {101, 0, SemanticAction::ChordMaj, true, false},
    {114, 0, SemanticAction::ChordSus, true, false},
    {97, 0, SemanticAction::Extension6, true, false},
    {115, 0, SemanticAction::ExtensionMinor7, true, false},
    {100, 0, SemanticAction::ExtensionMajor7, true, false},
    {102, 0, SemanticAction::Extension9, true, false},
    {91, 0, SemanticAction::VoicingDown, false, false},
    {93, 0, SemanticAction::VoicingUp, false, false},
    {109, 0, SemanticAction::ModeNext, false, false},
    {107, 0, SemanticAction::KeyToggle, false, false},
    {112, 0, SemanticAction::PerformanceNext, false, false},
    {98, 0, SemanticAction::BassToggle, false, false},
    {116, 0, SemanticAction::TempoTap, false, false},
    {99, 0, SemanticAction::ClockToggle, false, false},
    {118, 0, SemanticAction::ViewNext, false, false},
    {104, 0, SemanticAction::HarmonicQuantizeToggle, false, false},
    {108, 0, SemanticAction::OutputLaneNext, false, false},
    {120, 0, SemanticAction::ExtensionStackToggle, false, false},
    {111, 0, SemanticAction::PadSetup, false, false},
    {122, 0, SemanticAction::LoopRecord, false, false},
    {32, 0, SemanticAction::LoopPlay, false, false},
    {9, 0, SemanticAction::OptionsToggle, false, false},
    {96, 1, SemanticAction::Panic, false, false},
    {107, 1, SemanticAction::KeyLearn, false, false},
    {115, 1, SemanticAction::PresetSave, false, false},
    {108, 1, SemanticAction::PresetLoad, false, false},
    {122, 1, SemanticAction::LoopOverdub, false, false},
    {117, 1, SemanticAction::LoopUndo, false, false},
    {8, 1, SemanticAction::LoopClear, false, false},
    {103, 1, SemanticAction::LearnControl, false, false},
    {59, 0, SemanticAction::MenuUp, false, true},
    {46, 0, SemanticAction::MenuDown, false, true},
    {44, 0, SemanticAction::MenuDecrease, false, true},
    {47, 0, SemanticAction::MenuIncrease, false, true},
    {13, 0, SemanticAction::MenuConfirm, false, true},
    {96, 0, SemanticAction::MenuBack, false, true},
};

constexpr bool keyBindingsUnique() {
    for (std::size_t i = 0; i < std::size(keyBindings); ++i)
        for (std::size_t j = i + 1; j < std::size(keyBindings); ++j)
            if (keyBindings[i].key == keyBindings[j].key && keyBindings[i].modifiers == keyBindings[j].modifiers) return false;
    return true;
}
static_assert(keyBindingsUnique(), "Conflicting Cardputer shortcuts");

class KeyDispatcher {
public:
    using Callback = void (*)(void*, const ActionEvent&, uint64_t);
    void update(const std::array<bool, 128>& keys, uint8_t modifiers, bool menu, uint64_t nowUs, Callback callback, void* context) {
        for (std::size_t key = 0; key < keys.size(); ++key) {
            if (previous_[key] && !keys[key] && held_[key] != SemanticAction::None) {
                if (callback) callback(context, {held_[key], 0, false, static_cast<uint16_t>(key)}, nowUs);
                held_[key] = SemanticAction::None;
            }
        }
        for (const auto& binding : keyBindings) {
            if (!keys[binding.key] || previous_[binding.key] || binding.modifiers != modifiers) continue;
            if (binding.menu && !menu) continue;
            if (menu && !binding.menu && binding.action != SemanticAction::Panic && binding.action != SemanticAction::OptionsToggle) continue;
            if (binding.release) held_[binding.key] = binding.action;
            if (callback) callback(context, {binding.action, 1, true, binding.key}, nowUs);
        }
        previous_ = keys;
    }
private:
    std::array<bool, 128> previous_{};
    std::array<SemanticAction, 128> held_{};
};

}
