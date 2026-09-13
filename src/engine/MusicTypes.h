#pragma once

#include "common/FixedList.h"

#include <cstdint>

namespace midibrain {

using NoteList = FixedList<uint8_t, 32>;

enum class ChordQuality : uint8_t {
    Major,
    Minor,
    Diminished,
    Suspended
};

enum Extension : uint8_t {
    Extension6 = 1U << 0U,
    ExtensionMinor7 = 1U << 1U,
    ExtensionMajor7 = 1U << 2U,
    Extension9 = 1U << 3U
};

enum class ScaleType : uint8_t {
    Major,
    NaturalMinor,
    Dorian,
    Mixolydian,
    HarmonicMinor,
    MajorPentatonic,
    MinorPentatonic,
    Chromatic,
    Hijaz,
    Hijazkar,
    Kurd,
    Nikriz,
    Count
};

inline constexpr uint8_t ScaleCount = static_cast<uint8_t>(ScaleType::Count);

enum class PerformanceMode : uint8_t {
    Block,
    Strum,
    StrumTwoOctaves,
    Slop,
    Arp,
    ArpTwoOctaves,
    Pattern,
    Harp
};

enum class Direction : uint8_t {
    Up,
    Down,
    UpDown,
    Random
};

enum class Division : uint8_t {
    Quarter,
    Eighth,
    EighthTriplet,
    Sixteenth,
    SixteenthTriplet,
    ThirtySecond
};

}
