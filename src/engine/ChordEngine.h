#pragma once

#include "engine/MusicTypes.h"

#include <cstddef>
#include <cstdint>

namespace midibrain {

class ChordEngine {
public:
    NoteList build(uint8_t root, ChordQuality quality, uint8_t extensions) const;
    static void addExtensions(NoteList& notes, uint8_t root, uint8_t extensions);
    std::size_t formatName(char* destination, std::size_t capacity, uint8_t root, ChordQuality quality, uint8_t extensions) const;
};

}
