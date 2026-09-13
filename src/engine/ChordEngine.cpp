#include "engine/ChordEngine.h"

#include <array>
#include <cstdio>
#include <cstring>

namespace midibrain {

namespace {

uint8_t foldMidiPitch(int pitch) {
    while (pitch > 127) {
        pitch -= 12;
    }
    while (pitch < 0) {
        pitch += 12;
    }
    return static_cast<uint8_t>(pitch);
}

void appendUnique(NoteList& notes, uint8_t note) {
    for (const uint8_t existing : notes) {
        if (existing == note) {
            return;
        }
    }
    notes.push_back(note);
}

void appendText(char* destination, std::size_t capacity, const char* text) {
    const std::size_t used = std::strlen(destination);
    if (used + 1 >= capacity) {
        return;
    }
    std::snprintf(destination + used, capacity - used, "%s", text);
}

}

NoteList ChordEngine::build(uint8_t root, ChordQuality quality, uint8_t extensions) const {
    static constexpr std::array<std::array<uint8_t, 3>, 4> formulas{{
        {{0, 4, 7}},
        {{0, 3, 7}},
        {{0, 3, 6}},
        {{0, 5, 7}}
    }};
    NoteList notes;
    for (const uint8_t interval : formulas[static_cast<std::size_t>(quality)]) {
        appendUnique(notes, foldMidiPitch(static_cast<int>(root) + interval));
    }
    addExtensions(notes, root, extensions);
    return notes;
}

void ChordEngine::addExtensions(NoteList& notes, uint8_t root, uint8_t extensions) {
    constexpr std::array<std::pair<uint8_t, uint8_t>, 4> additions{{
        {Extension6, 9},
        {ExtensionMinor7, 10},
        {ExtensionMajor7, 11},
        {Extension9, 14}
    }};
    for (const auto& addition : additions) {
        if ((extensions & addition.first) != 0) {
            appendUnique(notes, foldMidiPitch(static_cast<int>(root) + addition.second));
        }
    }
    notes.sort([](uint8_t left, uint8_t right) { return left < right; });
}

std::size_t ChordEngine::formatName(char* destination, std::size_t capacity, uint8_t root, ChordQuality quality, uint8_t extensions) const {
    static constexpr std::array<const char*, 12> names{{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}};
    if (capacity == 0) {
        return 0;
    }
    destination[0] = '\0';
    appendText(destination, capacity, names[root % 12]);
    if (quality == ChordQuality::Minor) {
        appendText(destination, capacity, "m");
    } else if (quality == ChordQuality::Diminished) {
        appendText(destination, capacity, "dim");
    } else if (quality == ChordQuality::Suspended) {
        appendText(destination, capacity, "sus4");
    }
    const bool hasMinor7 = (extensions & ExtensionMinor7) != 0;
    const bool hasMajor7 = (extensions & ExtensionMajor7) != 0;
    const bool hasNine = (extensions & Extension9) != 0;
    if ((extensions & Extension6) != 0) {
        appendText(destination, capacity, "6");
    }
    if (hasMinor7 && !hasMajor7 && !hasNine) {
        appendText(destination, capacity, "7");
    }
    if (hasMajor7 && !hasNine) {
        appendText(destination, capacity, "maj7");
        if (hasMinor7) appendText(destination, capacity, "(b7)");
    }
    if (hasNine) {
        if (hasMajor7) appendText(destination, capacity, "maj9");
        else if (hasMinor7) appendText(destination, capacity, "9");
        else appendText(destination, capacity, "(add9)");
        if (hasMajor7 && hasMinor7) appendText(destination, capacity, "(b7)");
    }
    return std::strlen(destination);
}

}
