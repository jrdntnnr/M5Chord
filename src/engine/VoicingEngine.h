#pragma once

#include "engine/MusicTypes.h"

namespace midibrain {

class VoicingEngine {
public:
    NoteList apply(const NoteList& chord, int step) const;
    NoteList expandOctaves(const NoteList& chord, uint8_t octaveCount) const;
};

}

