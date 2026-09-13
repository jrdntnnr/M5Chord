#include "hardware/InputKeys.h"
#include <algorithm>
#include <cstdio>
#ifdef ARDUINO
#include <M5Cardputer.h>
#endif

namespace midibrain {
namespace {
struct MenuCommand { const char* name; SemanticAction action; };
constexpr MenuCommand commands[]{
    {"Learn key root",SemanticAction::KeyLearn},
    {"Learn control",SemanticAction::LearnControl},
    {"Pad setup",SemanticAction::PadSetup},
    {"Mapping / delete",SemanticAction::MappingDelete},
    {"Select profile",SemanticAction::ProfileNext},
    {"Reload profile",SemanticAction::ProfileReload},
    {"Save profile",SemanticAction::ProfileSave},
    {"Save preset",SemanticAction::PresetSave},
    {"Load preset",SemanticAction::PresetLoad},
    {"Loop record",SemanticAction::LoopRecord},
    {"Loop play / stop",SemanticAction::LoopPlay},
    {"Loop overdub",SemanticAction::LoopOverdub},
    {"Loop undo",SemanticAction::LoopUndo},
    {"Loop clear",SemanticAction::LoopClear},
    {"Save loop",SemanticAction::LoopSave},
    {"Load loop",SemanticAction::LoopLoad},
    {"Export diagnostics",SemanticAction::DiagnosticsExport},
    {"Edit mapping",SemanticAction::MappingEdit},
    {"Help / shortcuts",SemanticAction::HelpToggle},
    {"BLE reconnect",SemanticAction::BleReconnect},
    {"MIDI Player",SemanticAction::MidiFilePanel}
};
constexpr std::size_t rows = std::size(parameters) + std::size(commands);
}
InputKeys::InputKeys(App& app, ControllerMapper& mapper, ProfileStore& profiles, ActionCallback callback, void* context)
    : app_(app), mapper_(mapper), profiles_(profiles), callback_(callback), context_(context) {}
void InputKeys::selectMidiPlayback() {
    browsing_files_ = false; menu_ = true; player_page_ = true; help_ = false; editing_ = false;
    player_button_ = app_.midiFile().file().valid() ? 1 : 0;
}
void InputKeys::dispatch(SemanticAction action, uint64_t nowUs, int16_t value, bool pressed) {
    if (callback_) callback_(context_,{action,value,pressed},nowUs);
}
void InputKeys::beginPadSetup(uint64_t nowUs) {
    app_.panic(nowUs);
    mapper_.resetEdges();
    learner_.begin();
    generic_learn_ = false;
    editing_ = false;
    menu_ = false;
    learn_status_ = "WAITING FOR PAD";
}
void InputKeys::poll(uint64_t nowUs) {
#ifdef ARDUINO
    const auto& state = M5Cardputer.Keyboard.keysState();
    std::array<bool,128> keys{};
    for (const auto& point : M5Cardputer.Keyboard.keyList()) {
        const uint8_t value = static_cast<uint8_t>(M5Cardputer.Keyboard.getKeyValue(point).value_first);
        if (value < keys.size()) keys[value] = true;
    }
    keys[9] = state.tab;
    keys[13] = state.enter;
    keys[8] = state.del;
    const uint8_t modifiers = (state.fn ? 1 : 0) | (state.shift ? 2 : 0) | (state.ctrl ? 4 : 0) | (state.alt || state.opt ? 8 : 0);
    keyboard_.update(keys,modifiers,menuOpen() || learning(),nowUs,callback_,context_);
#else
    static_cast<void>(nowUs);
#endif
}
bool InputKeys::handleAction(const ActionEvent& event, uint64_t nowUs) {
    const auto action = event.action;
    if (!event.pressed) return false;
    if (action == SemanticAction::MidiFilePanel) { selectMidiPlayback(); return true; }
    if (browsing_files_) {
        if (action == SemanticAction::HelpToggle || action == SemanticAction::MenuBack || action == SemanticAction::OptionsToggle) {
            browsing_files_ = false; player_page_ = true; menu_ = action != SemanticAction::OptionsToggle; return true;
        }
        if (action == SemanticAction::MenuUp || action == SemanticAction::MenuDown || action == SemanticAction::MenuDecrease || action == SemanticAction::MenuIncrease) {
            const auto count = files_.names.size();
            if (count) file_index_ = (file_index_ + ((action == SemanticAction::MenuUp || action == SemanticAction::MenuDecrease) ? count - 1 : 1)) % count;
            return true;
        }
        if (action == SemanticAction::MenuConfirm) {
            if (!files_.names.empty()) { browsing_files_ = false; dispatch(SemanticAction::MidiFileLoad, nowUs, file_index_); }
            return true;
        }
    }
    if (playerOpen()) {
        if (action == SemanticAction::HelpToggle || action == SemanticAction::MenuBack || action == SemanticAction::OptionsToggle) {
            player_page_ = false; menu_ = action != SemanticAction::OptionsToggle; return true;
        }
        if (action == SemanticAction::MenuUp || action == SemanticAction::MenuDown || action == SemanticAction::MenuDecrease || action == SemanticAction::MenuIncrease) {
            player_button_ ^= 1; return true;
        }
        if (action == SemanticAction::MenuConfirm) {
            if (!file_loading_) dispatch(player_button_ == 0 ? SemanticAction::MidiFileBrowse : SemanticAction::MidiFilePlay, nowUs);
            return true;
        }
    }
    if (action == SemanticAction::HelpToggle) {
        if (learning() || editing_) {
            learner_.cancel(); generic_learn_ = false; pending_pads_ = false; editing_ = false; menu_ = false;
        } else { help_ = !help_; help_page_ = 0; }
        return true;
    }
    if (help_) {
        if (action == SemanticAction::MenuUp || action == SemanticAction::MenuDecrease)
            help_page_ = (help_page_ + HelpPages - 1) % HelpPages;
        else if (action == SemanticAction::MenuDown || action == SemanticAction::MenuIncrease || action == SemanticAction::MenuConfirm)
            help_page_ = (help_page_ + 1) % HelpPages;
        else if (action == SemanticAction::OptionsToggle) { help_ = false; menu_ = !menu_; }
        else if (action == SemanticAction::MenuBack) help_ = false;
        else return false;
        return true;
    }
    if (action == SemanticAction::PadSetup) { beginPadSetup(nowUs); return true; }
    if (action == SemanticAction::OptionsToggle || action == SemanticAction::MenuBack) {
        if (learning()) { learner_.cancel(); generic_learn_ = false; }
        pending_pads_ = false;
        editing_ = false;
        menu_ = action == SemanticAction::MenuBack ? false : !menu_;
        return true;
    }
    if (action == SemanticAction::LearnControl) {
        app_.panic(nowUs);
        mapper_.resetEdges();
        learner_.cancel();
        generic_learn_ = true;
        menu_ = false;
        edit_ = {};
        edit_existing_ = false;
        edit_.consume = true;
        edit_.action = SemanticAction::VoicingUp;
        learn_status_ = "MOVE A MIDI CONTROL";
        return true;
    }
    if (action == SemanticAction::ProfileNext || action == SemanticAction::ProfileReload || action == SemanticAction::ProfileSave) {
        app_.panic(nowUs);
        mapper_.resetEdges();
        storage_action_ = action;
        return true;
    }
    if (action == SemanticAction::MappingEdit) {
        if (mapping_index_ < mapper_.size()) {
            app_.panic(nowUs);
            mapper_.resetEdges();
            edit_ = mapper_.mapping(mapping_index_);
            edit_existing_ = true;
            editing_ = true;
            menu_ = true;
            edit_field_ = 0;
        }
        return true;
    }
    if (action == SemanticAction::MappingDelete) {
        app_.panic(nowUs);
        if (mapper_.erase(mapping_index_)) {
            std::snprintf(status_,sizeof(status_),"Mapping deleted; save profile");
            if (mapping_index_ >= mapper_.size()) mapping_index_ = 0;
        }
        return true;
    }
    if (action == SemanticAction::MenuUp || action == SemanticAction::MenuDown) {
        if (editing_) edit_field_ = static_cast<uint8_t>((edit_field_ + (action == SemanticAction::MenuUp ? 4 : 1)) % 5);
        else selected_ = (selected_ + (action == SemanticAction::MenuUp ? rows - 1 : 1)) % rows;
        return true;
    }
    if (action == SemanticAction::MenuDecrease || action == SemanticAction::MenuIncrease || action == SemanticAction::MenuConfirm) {
        if (!menu_) return true;
        if (editing_) {
            const int direction = action == SemanticAction::MenuDecrease ? -1 : 1;
            if (edit_field_ == 0) {
                int next = static_cast<int>(edit_.action) + direction;
                if (next <= 0) next = static_cast<int>(SemanticAction::Count) - 1;
                if (next >= static_cast<int>(SemanticAction::Count)) next = 1;
                edit_.action = static_cast<SemanticAction>(next);
            } else if (edit_field_ == 1) edit_.trigger = static_cast<MappingTrigger>((static_cast<int>(edit_.trigger)+6+direction)%6);
            else if (edit_field_ == 2) edit_.relative_mode = static_cast<RelativeMode>((static_cast<int>(edit_.relative_mode)+3+direction)%3);
            else if (edit_field_ == 3) edit_.consume = !edit_.consume;
            else if (action == SemanticAction::MenuConfirm) {
                bool duplicate = false;
                for (std::size_t i=0;i<mapper_.size();++i) {
                    if (edit_existing_ && i == mapping_index_) continue;
                    const auto& m=mapper_.mapping(i);
                    duplicate |= m.source.type==edit_.source.type && m.source.channel==edit_.source.channel && m.source.number==edit_.source.number && m.source.cable==edit_.source.cable;
                }
                if (duplicate) std::snprintf(status_,sizeof(status_),"Duplicate: delete old mapping");
                else if (edit_existing_ ? mapper_.replace(mapping_index_,edit_) : mapper_.add(edit_)) {
                    editing_ = false;
                    storage_action_ = SemanticAction::ProfileSave;
                    std::snprintf(status_,sizeof(status_),"Mapping added; save queued");
                } else std::snprintf(status_,sizeof(status_),"Mapping list full");
            }
        } else if (selected_ >= std::size(parameters) && action != SemanticAction::MenuConfirm) {
            const auto command = commands[selected_ - std::size(parameters)].action;
            if ((command == SemanticAction::MappingDelete || command == SemanticAction::MappingEdit) && mapper_.size())
                mapping_index_ = (mapping_index_ + (action == SemanticAction::MenuDecrease ? mapper_.size()-1 : 1)) % mapper_.size();
        } else activateMenu(action == SemanticAction::MenuDecrease ? -1 : 1,nowUs);
        return true;
    }
    return false;
}
void InputKeys::activateMenu(int delta, uint64_t nowUs) {
    if (selected_ < std::size(parameters)) {
        const auto& p = parameters[selected_];
        dispatch(p.action,nowUs,p.cycle ? delta : parameterValue(selected_,app_.state())+delta);
    } else dispatch(commands[selected_-std::size(parameters)].action,nowUs);
}
bool InputKeys::captureLearnEvent(const MidiEvent& event) {
    if (pending_pads_ || editing_) return true;
    if (generic_learn_) {
        if (event.type != MidiType::NoteOn && event.type != MidiType::ControlChange && event.type != MidiType::ProgramChange && event.type != MidiType::PitchBend && event.type != MidiType::ChannelPressure) return true;
        if ((event.type == MidiType::NoteOn || event.type == MidiType::ControlChange) && !event.data2) return true;
        edit_.source = {event.type,static_cast<int8_t>(event.channel),static_cast<int16_t>(event.type == MidiType::PitchBend || event.type == MidiType::ChannelPressure ? -1 : event.data1),0,127,static_cast<int8_t>(event.cable)};
        edit_.trigger = event.type == MidiType::NoteOn ? MappingTrigger::PressRelease : MappingTrigger::Value;
        generic_learn_ = false;
        editing_ = true;
        menu_ = true;
        edit_field_ = 0;
        std::snprintf(status_,sizeof(status_),"%s %d Ch%u cable%u",event.type == MidiType::NoteOn ? "NOTE" : event.type == MidiType::ControlChange ? "CC" : "MIDI",edit_.source.number,event.channel+1,event.cable);
        return true;
    }
    if (!learner_.active()) return false;
    const auto result = learner_.capture(event);
    if (result == PadLearnResult::Guarded) learn_status_ = "RELEASE; WAIT FOR NEXT PAD";
    else if (result == PadLearnResult::Duplicate) learn_status_ = "DUPLICATE CONTROL";
    else if (result == PadLearnResult::Captured) learn_status_ = "CONTROL CAPTURED";
    else if (result == PadLearnResult::Complete) {
        pending_pads_ = true;
        learn_status_ = "SAVE PENDING";
    }
    return true;
}
void InputKeys::service(uint64_t nowUs) {
    if (!app_.idle()) return;
    if (pending_pads_) {
        if (profiles_.saveSmk37(learner_.mappings())) {
            mapper_.clear();
            for (const auto& mapping : learner_.mappings()) mapper_.add(mapping);
            std::snprintf(status_,sizeof(status_),"9 PADS SAVED");
        } else std::snprintf(status_,sizeof(status_),"%s",profiles_.error());
        pending_pads_ = false;
    }
    if (storage_action_ != SemanticAction::None) {
        const auto action = storage_action_;
        storage_action_ = SemanticAction::None;
        app_.panic(nowUs);
        if (action == SemanticAction::ProfileSave) profiles_.saveActive(mapper_,app_.state());
        else {
            if (action == SemanticAction::ProfileNext) profiles_.next(mapper_);
            else profiles_.loadActive(mapper_);
            profiles_.applyInput(app_.state());
        }
        std::snprintf(status_,sizeof(status_),"%s",profiles_.error()[0] ? profiles_.error() : "PROFILE READY");
    }
}
bool InputKeys::learning() const { return learner_.active() || generic_learn_ || pending_pads_; }
uint8_t InputKeys::learnStep() const { return learner_.step(); }
const char* InputKeys::learnActionName() const { return generic_learn_ ? "MIDI LEARN" : ControllerMapper::actionName(learner_.currentAction()); }
const char* InputKeys::learnStatus() const { return learn_status_; }
bool InputKeys::menuOpen() const { return menu_ || help_; }
const char* InputKeys::status() const { return status_; }
std::size_t InputKeys::menuIndex() const { return browsing_files_ ? file_index_ : editing_ ? edit_field_ : selected_; }
std::size_t InputKeys::menuCount() const { return browsing_files_ ? std::max<std::size_t>(1, files_.names.size()) : editing_ ? 5 : rows; }
const char* InputKeys::menuNeighbor(int direction) const {
    if (editing_) return direction < 0 ? "Edit MIDI mapping" : "Finish on Save mapping";
    const auto row = (selected_ + rows + direction) % rows;
    return row < std::size(parameters) ? parameters[row].label : commands[row - std::size(parameters)].name;
}
const char* InputKeys::menuHelp() const {
    if (browsing_files_) return files_.truncated ? "List limited; Enter loads; Esc back" : "Enter loads; Esc back; files in /midi";
    if (selected_ >= std::size(parameters) && commands[selected_ - std::size(parameters)].action == SemanticAction::MidiFilePlay)
        return app_.midiFile().name()[0] ? app_.midiFile().name() : "Load a file from /midi first";
    if (editing_) return edit_field_ == 4 ? "Enter saves; Tab cancels" : "Choose how this control behaves";
    if (selected_ >= std::size(parameters)) return "Enter runs this command";
    switch (selected_) {
        case 0: return "KEY: auto chords. CHORD: choose type.";
        case 1:
            switch (app_.state().harmonic.play_style) {
                case PlayStyle::Simple: return "Hold type before playing a note";
                case PlayStyle::Advanced: return "First type stays until note release";
                case PlayStyle::Free: return "Change type while a note is held";
                case PlayStyle::Latched: return "Tap type; selection stays on";
            }
            break;
        case 2: return app_.state().harmonic.extension_stack ? "Press an extension to toggle it" : "Extensions last while you hold them";
        case 3: return "Keep common notes or attack again";
        case 4: return "Root of the selected scale";
        case 5: return app_.state().mode == EngineMode::Bypass ? "BYPASS: scale has no effect"
            : app_.state().mode == EngineMode::Chord ? (app_.state().harmonic.harmonic_quantize ? "CHORD: chord tones snap to this scale" : "CHORD: turn Harmonic quantize ON")
            : "KEY sets the triad; extensions add notes";
        case 8: return app_.state().mode == EngineMode::Bypass ? "Choose CHORD or KEY to hear this" : "How the generated chord is played";
        case 10: return "Note spacing relative to BPM";
        case 11: return "Length of each arp or pattern note";
        case 15: return "Send MIDI clock to the receiver";
        case 16: return "Main MIDI output channel 1-16";
        case 33: return "AUTO locks to first active input";
        case 34: return "L cycles channels 1 through this value";
        case 27: return "Recording length; FREE ends on press";
        case 29: return "Destination for preset and loop files";
        case 30: return app_.state().mode == EngineMode::Key ? "KEY already builds scale-based triads" : "Fit chord tones to the selected scale";
        default: return "Changes apply as you adjust";
    }
    return "Changes apply as you adjust";
}
void InputKeys::menuText(char* title, std::size_t titleSize, char* value, std::size_t valueSize) const {
    if (browsing_files_) {
        std::snprintf(title, titleSize, "LOAD MIDI /midi");
        std::snprintf(value, valueSize, "%s", files_.names.empty() ? "NO MIDI FILES" : files_.names[file_index_].data());
        return;
    }
    if (editing_) {
        const char* labels[]{"Action","Trigger","Relative encoding","Consume MIDI","Save mapping"};
        const char* triggers[]{"PRESS","RELEASE","PRESS+RELEASE","TOGGLE","VALUE","RELATIVE"};
        const char* relatives[]{"TWOS COMPLEMENT","BINARY OFFSET","SIGNED BIT"};
        std::snprintf(title,titleSize,"LEARN: %s",labels[edit_field_]);
        const char* result = edit_field_ == 0 ? ControllerMapper::actionName(edit_.action) : edit_field_ == 1 ? triggers[static_cast<int>(edit_.trigger)] : edit_field_ == 2 ? relatives[static_cast<int>(edit_.relative_mode)] : edit_field_ == 3 ? (edit_.consume ? "YES" : "NO") : "ENTER TO SAVE";
        std::snprintf(value,valueSize,"%s",result);
    } else if (selected_ < std::size(parameters)) {
        std::snprintf(title,titleSize,"%s",parameters[selected_].label);
        formatParameter(selected_,app_.state(),value,valueSize);
    } else {
        const auto& command = commands[selected_-std::size(parameters)];
        std::snprintf(title,titleSize,"%s",command.name);
        if ((command.action == SemanticAction::MappingDelete || command.action == SemanticAction::MappingEdit) && mapper_.size()) {
            const auto& m = mapper_.mapping(mapping_index_);
            std::snprintf(value,valueSize,"%u Ch%d #%d %s",static_cast<unsigned>(mapping_index_+1),m.source.channel+1,m.source.number,ControllerMapper::actionName(m.action));
        } else if (command.action == SemanticAction::MidiFilePlay) {
            std::snprintf(value, valueSize, "%s / %u CH", app_.midiFile().status(), app_.state().lane_count);
        } else std::snprintf(value,valueSize,"%s",command.action == SemanticAction::ProfileNext ? profiles_.name() : "ENTER");
    }
}
}
