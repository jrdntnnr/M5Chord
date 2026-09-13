#pragma once

#include "engine/MusicTypes.h"

#include <array>
#include <cstdint>

namespace midibrain {

struct ScaleDefinition {
    const char* name;
    std::array<uint8_t, 12> semitones;
    uint8_t length;
};

class ScaleEngine {
public:
    const ScaleDefinition& definition(ScaleType type) const;
    bool contains(uint8_t keyRoot, ScaleType type, uint8_t note) const;
    NoteList buildDiatonic(uint8_t inputRoot, uint8_t keyRoot, ScaleType type, uint8_t extensions = 0) const;
    NoteList quantizeChord(const NoteList& chord, uint8_t keyRoot, ScaleType type) const;
};

}
