#pragma once
#include "app/App.h"
#include "controller/KeyBindings.h"
#include "controller/PadLearner.h"
#include "controller/Parameters.h"
#include "controller/ShortcutHelp.h"
#include "storage/ProfileStore.h"
#include "storage/MidiFileStore.h"
#include <cstdint>

namespace midibrain {
class InputKeys {
public:
    using ActionCallback = void (*)(void*, const ActionEvent&, uint64_t);
    InputKeys(App& app, ControllerMapper& mapper, ProfileStore& profiles, ActionCallback callback, void* context);
    void poll(uint64_t nowUs);
    bool handleAction(const ActionEvent& action, uint64_t nowUs);
    void service(uint64_t nowUs);
    bool captureLearnEvent(const MidiEvent& event);
    bool learning() const;
    uint8_t learnStep() const;
    const char* learnActionName() const;
    const char* learnStatus() const;
    bool menuOpen() const;
    bool helpOpen() const { return help_; }
    std::size_t helpPage() const { return help_page_; }
    void menuText(char* title, std::size_t titleSize, char* value, std::size_t valueSize) const;
    const char* status() const;
    const char* menuHelp() const;
    std::size_t menuIndex() const;
    std::size_t menuCount() const;
    const char* menuNeighbor(int direction) const;
    MidiFileCatalog& fileCatalog() { return files_; }
    void openFiles() { browsing_files_ = true; file_index_ = 0; menu_ = true; help_ = false; editing_ = false; }
    bool browsingFiles() const { return browsing_files_; }
    void selectMidiPlayback();
    bool playerOpen() const { return player_page_ && menu_ && !browsing_files_; }
    uint8_t playerButton() const { return player_button_; }
    void setFileLoading(bool loading, uint8_t progress) { file_loading_ = loading; file_progress_ = progress; }
    bool fileLoading() const { return file_loading_; }
    uint8_t fileProgress() const { return file_progress_; }
private:
    void dispatch(SemanticAction action, uint64_t nowUs, int16_t value = 1, bool pressed = true);
    void beginPadSetup(uint64_t nowUs);
    void activateMenu(int delta, uint64_t nowUs);
    App& app_;
    ControllerMapper& mapper_;
    ProfileStore& profiles_;
    ActionCallback callback_;
    void* context_;
    PadLearner learner_{};
    KeyDispatcher keyboard_{};
    const char* learn_status_{""};
    bool menu_{false};
    bool help_{false};
    std::size_t help_page_{0};
    bool generic_learn_{false};
    bool editing_{false};
    bool edit_existing_{false};
    bool pending_pads_{false};
    SemanticAction storage_action_{SemanticAction::None};
    std::size_t selected_{0};
    std::size_t mapping_index_{0};
    uint8_t edit_field_{0};
    ControllerMapping edit_{};
    char status_[96]{};
    MidiFileCatalog files_{};
    bool browsing_files_{false};
    bool player_page_{false}, file_loading_{false};
    uint8_t player_button_{0}, file_progress_{0};
    std::size_t file_index_{0};
};
}
