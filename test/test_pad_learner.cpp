#include "TestSupport.h"
#include "controller/PadLearner.h"

using namespace midibrain;

void testPadLearner() {
    PadLearner learner;
    learner.begin();
    EXPECT(learner.active());
    EXPECT(learner.currentAction() == SemanticAction::ChordDim);

    EXPECT(learner.capture(MidiEvent::noteOn(9, 36, 100, 1000000)) == PadLearnResult::Captured);
    EXPECT(learner.step() == 1);
    EXPECT(learner.currentAction() == SemanticAction::ChordMin);
    EXPECT(learner.capture(MidiEvent::noteOn(9, 37, 100, 1100000)) == PadLearnResult::Guarded);
    EXPECT(learner.step() == 1);
    EXPECT(learner.capture(MidiEvent::noteOff(9, 36, 0, 1200000)) == PadLearnResult::Guarded);
    EXPECT(learner.capture(MidiEvent::noteOn(9, 36, 100, 1400000)) == PadLearnResult::Duplicate);
    EXPECT(learner.step() == 1);

    for (uint8_t index = 1; index < PadLearner::MappingCount; ++index) {
        const MidiEvent event = MidiEvent::noteOn(9, static_cast<uint8_t>(36 + index), 100, 1000000ULL + index * 400000ULL);
        const PadLearnResult expected = index + 1 == PadLearner::MappingCount ? PadLearnResult::Complete : PadLearnResult::Captured;
        EXPECT(learner.capture(event) == expected);
        learner.capture(MidiEvent::noteOff(9, static_cast<uint8_t>(36 + index), 0, event.timestamp_us + 100000));
    }
    EXPECT(!learner.active());
    EXPECT(learner.mappings().size() == PadLearner::MappingCount);
    EXPECT(learner.mappings()[8].action == SemanticAction::OutputLaneNext);

    learner.begin();
    MidiEvent cc = MidiEvent::cc(9, 20, 127, 5000000);
    EXPECT(learner.capture(cc) == PadLearnResult::Captured);
    EXPECT(learner.mappings()[0].trigger == MappingTrigger::PressRelease);
}
