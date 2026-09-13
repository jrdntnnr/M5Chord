#pragma once
#include "app/App.h"
#include "controller/KeyBindings.h"
#include "controller/PadLearner.h"
#include "controller/Parameters.h"
#include "storage/ProfileStore.h"
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
    void menuText(char* title, std::size_t titleSize, char* value, std::size_t valueSize) const;
    const char* status() const;
    const char* menuHelp() const;
    std::size_t menuIndex() const;
    std::size_t menuCount() const;
    const char* menuNeighbor(int direction) const;
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
};
}
