#include "TestSupport.h"
#include "engine/VoicingEngine.h"

using namespace midibrain;

void testVoicing() {
    NoteList chord;
    chord.push_back(60);
    chord.push_back(64);
    chord.push_back(67);
    VoicingEngine engine;
    const auto upOne = engine.apply(chord, 1);
    EXPECT(upOne[0] == 64 && upOne[1] == 67 && upOne[2] == 72);
    const auto upTwo = engine.apply(chord, 2);
    EXPECT(upTwo[0] == 67 && upTwo[1] == 72 && upTwo[2] == 76);
    const auto downOne = engine.apply(chord, -1);
    EXPECT(downOne[0] == 55 && downOne[1] == 60 && downOne[2] == 64);
}
