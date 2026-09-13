#include "TestSupport.h"
#include "controller/ControllerMapper.h"

using namespace midibrain;

void testMapper() {
    ControllerMapper mapper;
    ControllerMapping mapping;
    mapping.source = {MidiType::NoteOn, 9, 36, 0, 127, -1};
    mapping.trigger = MappingTrigger::PressRelease;
    mapping.action = SemanticAction::ChordDim;
    mapping.consume = true;
    EXPECT(mapper.add(mapping));
    auto result = mapper.map(MidiEvent::noteOn(9, 36, 100));
    EXPECT(result.consumed && result.actions.size() == 1 && result.actions[0].pressed);
    result = mapper.map(MidiEvent::noteOff(9, 36));
    EXPECT(result.consumed && result.actions.size() == 1 && !result.actions[0].pressed);
    EXPECT(ControllerMapper::actionFromName("extension.M7") == SemanticAction::ExtensionMajor7);
    EXPECT(ControllerMapper::actionFromName("extension.stack.toggle") == SemanticAction::ExtensionStackToggle);
    EXPECT(ControllerMapper::actionFromName("invalid") == SemanticAction::None);
    mapper.clear();
    mapping.trigger = MappingTrigger::Release;
    mapping.action = SemanticAction::OutputLaneNext;
    mapper.add(mapping);
    EXPECT(mapper.map(MidiEvent::noteOn(9,36,100)).actions.empty());
    result = mapper.map(MidiEvent::noteOff(9,36));
    EXPECT(result.actions.size() == 1 && result.actions[0].pressed);
    EXPECT(mapper.map(MidiEvent::noteOff(9,36)).actions.empty());
    for (int action=1;action<static_cast<int>(SemanticAction::Count);++action) {
        const auto semantic = static_cast<SemanticAction>(action);
        EXPECT(ControllerMapper::actionFromName(ControllerMapper::actionName(semantic)) == semantic);
    }
}
