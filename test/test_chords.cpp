#include "TestSupport.h"
#include "engine/ChordEngine.h"

#include <array>
#include <cstring>

using namespace midibrain;

namespace {

template <std::size_t N>
void expectNotes(const NoteList& actual, const std::array<uint8_t, N>& expected) {
    EXPECT(actual.size() == expected.size());
    for (std::size_t i = 0; i < N; ++i) EXPECT(actual[i] == expected[i]);
}

}

void testChords() {
    ChordEngine engine;
    expectNotes(engine.build(60, ChordQuality::Major, 0), std::array<uint8_t, 3>{60, 64, 67});
    expectNotes(engine.build(60, ChordQuality::Minor, 0), std::array<uint8_t, 3>{60, 63, 67});
    expectNotes(engine.build(60, ChordQuality::Diminished, 0), std::array<uint8_t, 3>{60, 63, 66});
    expectNotes(engine.build(60, ChordQuality::Suspended, 0), std::array<uint8_t, 3>{60, 65, 67});
    expectNotes(engine.build(60, ChordQuality::Major, ExtensionMajor7 | Extension9), std::array<uint8_t, 5>{60, 64, 67, 71, 74});
    expectNotes(engine.build(60, ChordQuality::Minor, ExtensionMinor7 | Extension9), std::array<uint8_t, 5>{60, 63, 67, 70, 74});
    char name[32]{};
    engine.formatName(name, sizeof(name), 60, ChordQuality::Major, ExtensionMajor7 | Extension9);
    EXPECT(std::strcmp(name, "Cmaj9") == 0);
    engine.formatName(name, sizeof(name), 60, ChordQuality::Major, ExtensionMajor7 | ExtensionMinor7);
    EXPECT(std::strcmp(name, "Cmaj7(b7)") == 0);
}
