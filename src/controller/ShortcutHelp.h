#pragma once
#include "controller/ControllerMapper.h"
#include <cstddef>

namespace midibrain {

struct Shortcut { SemanticAction action; const char* key; const char* description; };
inline constexpr Shortcut shortcuts[]{
    {SemanticAction::HelpToggle, "ESC", "Help / close help"},
    {SemanticAction::OptionsToggle, "TAB", "Options / close"},
    {SemanticAction::Panic, "FN ESC", "Panic: all notes off"},
    {SemanticAction::ModeNext, "M", "Bypass / Chord / Key"},
    {SemanticAction::KeyToggle, "K", "Key mode toggle"},
    {SemanticAction::ViewNext, "V", "Change display view"},
    {SemanticAction::ChordDim, "Q", "Diminished quality"},
    {SemanticAction::ChordMin, "W", "Minor quality"},
    {SemanticAction::ChordMaj, "E", "Major quality"},
    {SemanticAction::ChordSus, "R", "Suspended quality"},
    {SemanticAction::Extension6, "A", "Add sixth"},
    {SemanticAction::ExtensionMinor7, "S", "Add minor seventh"},
    {SemanticAction::ExtensionMajor7, "D", "Add major seventh"},
    {SemanticAction::Extension9, "F", "Add ninth"},
    {SemanticAction::ExtensionStackToggle, "X", "Hold / stack additions"},
    {SemanticAction::VoicingDown, "[", "Voicing down"},
    {SemanticAction::VoicingUp, "]", "Voicing up"},
    {SemanticAction::HarmonicQuantizeToggle, "H", "Harmonic quantize"},
    {SemanticAction::PerformanceNext, "P", "Block / strum / arp..."},
    {SemanticAction::BassToggle, "B", "Bass stream on/off"},
    {SemanticAction::TempoTap, "T", "Tap tempo"},
    {SemanticAction::ClockToggle, "C", "MIDI clock output"},
    {SemanticAction::OutputLaneNext, "L", "Next configured layer"},
    {SemanticAction::PadSetup, "O", "Nine-pad setup"},
    {SemanticAction::LearnControl, "FN G", "Learn MIDI control"},
    {SemanticAction::KeyLearn, "FN K", "Learn key root"},
    {SemanticAction::PresetSave, "FN S", "Save selected preset"},
    {SemanticAction::PresetLoad, "FN L", "Load selected preset"},
    {SemanticAction::LoopRecord, "Z", "Record / finish loop"},
    {SemanticAction::LoopPlay, "SPACE", "Loop play / stop"},
    {SemanticAction::LoopOverdub, "FN Z", "Loop overdub toggle"},
    {SemanticAction::LoopUndo, "FN U", "Stop / undo overdub"},
    {SemanticAction::LoopClear, "FN DEL", "Stop / clear loop"},
    {SemanticAction::MenuUp, ";", "Previous row / page"},
    {SemanticAction::MenuDown, ".", "Next row / page"},
    {SemanticAction::MenuDecrease, ",", "Decrease / prev page"},
    {SemanticAction::MenuIncrease, "/", "Increase / next page"},
    {SemanticAction::MenuConfirm, "ENTER", "Run command / page"},
};
inline constexpr std::size_t HelpRows = 6;
inline constexpr std::size_t HelpPages = (std::size(shortcuts) + HelpRows - 1) / HelpRows;

}
