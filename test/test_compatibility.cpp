#include "TestSupport.h"
#include "controller/KeyBindings.h"
#include "hardware/CardputerHardware.h"
#include "common/AppInfo.h"
#include <cstring>

using namespace midibrain;

namespace {
void capture(void* context, const ActionEvent& action, uint64_t) {
    static_cast<FixedList<ActionEvent, 64>*>(context)->push_back(action);
}
}

void testCompatibility() {
    EXPECT(std::strcmp(AppName, "M5Chord") == 0);
    EXPECT(std::strcmp(AppVersion, "1.1.0") == 0);
    EXPECT(CardputerPins::SdClock == 40);
    EXPECT(CardputerPins::SdMiso == 39);
    EXPECT(CardputerPins::SdMosi == 14);
    EXPECT(CardputerPins::SdSelect == 12);
    EXPECT(CardputerPins::MidiRx == 1);
    EXPECT(CardputerPins::MidiTx == 2);
    constexpr int matrixPins[]{3, 4, 5, 6, 7, 8, 9, 11, 13, 15};
    for (auto pin : matrixPins) {
        EXPECT(pin != CardputerPins::SdClock && pin != CardputerPins::SdMiso);
        EXPECT(pin != CardputerPins::SdMosi && pin != CardputerPins::SdSelect);
        EXPECT(pin != CardputerPins::MidiRx && pin != CardputerPins::MidiTx);
    }
    EXPECT(std::strcmp(cardputerFamilyName(CardputerFamily::Classic), "1.0/1.1") == 0);
    EXPECT(std::strcmp(cardputerFamilyName(CardputerFamily::Adv), "ADV") == 0);
    EXPECT(std::strcmp(cardputerFamilyName(CardputerFamily::Unsupported), "UNKNOWN") == 0);
    KeyDispatcher dispatcher;
    FixedList<ActionEvent, 64> actions;
    std::array<bool, 128> keys{};
    keys['s'] = true;
    dispatcher.update(keys, 0, false, 1000, capture, &actions);
    EXPECT(actions.size() == 1 && actions[0].action == SemanticAction::ExtensionMinor7);
    for (unsigned i = 0; i < 100; ++i) dispatcher.update(keys, 0, false, 1001 + i, capture, &actions);
    EXPECT(actions.size() == 1);
    keys['s'] = false;
    keys['d'] = true;
    dispatcher.update(keys, 0, false, 2000, capture, &actions);
    EXPECT(actions.size() == 3);
    EXPECT(actions[1].action == SemanticAction::ExtensionMinor7 && !actions[1].pressed);
    EXPECT(actions[2].action == SemanticAction::ExtensionMajor7 && actions[2].pressed);
    keys['d'] = false;
    keys['q'] = true;
    dispatcher.update(keys, 1, true, 3000, capture, &actions);
    EXPECT(actions.size() == 4);
    EXPECT(actions[3].action == SemanticAction::ExtensionMajor7 && !actions[3].pressed);
    keys.fill(false);
    dispatcher.update(keys, 0, false, 4000, capture, &actions);
    keys['s'] = true;
    dispatcher.update(keys, 1, false, 5000, capture, &actions);
    EXPECT(actions.size() == 5 && actions[4].action == SemanticAction::PresetSave);
    dispatcher.update(keys, 0, false, 5001, capture, &actions);
    EXPECT(actions.size() == 5);
    keys.fill(false);
    dispatcher.update(keys, 0, false, 5002, capture, &actions);
    EXPECT(actions.size() == 5);
    keys['q'] = true;
    dispatcher.update(keys, 0, false, 6000, capture, &actions);
    keys['q'] = false;
    keys['w'] = true;
    dispatcher.update(keys, 0, false, 6001, capture, &actions);
    EXPECT(actions[6].action == SemanticAction::ChordDim && !actions[6].pressed);
    EXPECT(actions[7].action == SemanticAction::ChordMin && actions[7].pressed);
    EXPECT(keyBindingsUnique());
}
