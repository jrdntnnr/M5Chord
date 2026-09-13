#include "engine/ScaleEngine.h"
#include "engine/ChordEngine.h"

#include <array>
#include <cstdlib>

namespace midibrain {

namespace {

constexpr std::array<ScaleDefinition, ScaleCount> scales{{
    {"Major", {0, 2, 4, 5, 7, 9, 11}, 7},
    {"Natural Minor", {0, 2, 3, 5, 7, 8, 10}, 7},
    {"Dorian", {0, 2, 3, 5, 7, 9, 10}, 7},
    {"Mixolydian", {0, 2, 4, 5, 7, 9, 10}, 7},
    {"Harmonic Minor", {0, 2, 3, 5, 7, 8, 11}, 7},
    {"Major Pentatonic", {0, 2, 4, 7, 9}, 5},
    {"Minor Pentatonic", {0, 3, 5, 7, 10}, 5},
    {"Chromatic", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, 12},
    {"Hijaz 12T", {0, 1, 4, 5, 7, 8, 10}, 7},
    {"Hijazkar 12T", {0, 1, 4, 5, 7, 8, 11}, 7},
    {"Kurd 12T", {0, 1, 3, 5, 7, 8, 10}, 7},
    {"Nikriz 12T", {0, 2, 3, 6, 7, 9, 10}, 7}
}};

int nearestDegree(const ScaleDefinition& scale, uint8_t keyRoot, uint8_t note) {
    int best = 0;
    int bestDistance = 128;
    for (uint8_t i = 0; i < scale.length; ++i) {
        const int pitchClass = (keyRoot + scale.semitones[i]) % 12;
        int distance = std::abs(static_cast<int>(note % 12) - pitchClass);
        distance = distance > 6 ? 12 - distance : distance;
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

uint8_t nearestInScaleAtOrAbove(int target, uint8_t keyRoot, const ScaleDefinition& scale, int minimum) {
    int best = target;
    int bestDistance = 129;
    for (int candidate = 0; candidate <= 127; ++candidate) {
        const uint8_t relative = static_cast<uint8_t>((candidate - keyRoot + 120) % 12);
        bool valid = false;
        for (uint8_t i = 0; i < scale.length; ++i) {
            valid = valid || scale.semitones[i] == relative;
        }
        if (!valid || candidate < minimum) {
            continue;
        }
        const int distance = std::abs(candidate - target);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = candidate;
        }
    }
    return static_cast<uint8_t>(best);
}

}

const ScaleDefinition& ScaleEngine::definition(ScaleType type) const {
    const auto index = static_cast<std::size_t>(type);
    return scales[index < scales.size() ? index : 0];
}

bool ScaleEngine::contains(uint8_t keyRoot, ScaleType type, uint8_t note) const {
    const auto& scale = definition(type);
    const uint8_t relative = static_cast<uint8_t>((note - keyRoot + 120) % 12);
    for (uint8_t i = 0; i < scale.length; ++i) {
        if (scale.semitones[i] == relative) {
            return true;
        }
    }
    return false;
}

NoteList ScaleEngine::buildDiatonic(uint8_t inputRoot, uint8_t keyRoot, ScaleType type, uint8_t extensions) const {
    const auto& scale = definition(type);
    const int rootDegree = nearestDegree(scale, keyRoot, inputRoot);
    int scaleBase = keyRoot;
    while (scaleBase + scale.semitones[rootDegree] > inputRoot) {
        scaleBase -= 12;
    }
    while (scaleBase + scale.semitones[rootDegree] + 12 <= inputRoot) {
        scaleBase += 12;
    }
    NoteList result;
    for (uint8_t i = 0; i < 3; ++i) {
        const int degree = rootDegree + i * 2;
        const int octave = degree / scale.length;
        const int index = degree % scale.length;
        int pitch = scaleBase + scale.semitones[index] + octave * 12;
        while (pitch > 127) {
            pitch -= 12;
        }
        while (pitch < 0) pitch += 12;
        if (pitch >= 0) {
            bool duplicate = false;
            for (const auto existing : result) duplicate = duplicate || existing == pitch;
            if (!duplicate) result.push_back(static_cast<uint8_t>(pitch));
        }
    }
    int root = scaleBase + scale.semitones[rootDegree];
    while (root < 0) root += 12;
    while (root > 127) root -= 12;
    ChordEngine::addExtensions(result, static_cast<uint8_t>(root), extensions);
    return result;
}

NoteList ScaleEngine::quantizeChord(const NoteList& chord, uint8_t keyRoot, ScaleType type) const {
    if (chord.empty()) {
        return {};
    }
    NoteList result;
    result.push_back(chord[0]);
    int minimum = chord[0] + 1;
    const auto& scale = definition(type);
    for (std::size_t i = 1; i < chord.size(); ++i) {
        const uint8_t pitch = contains(keyRoot, type, chord[i])
            ? static_cast<uint8_t>(chord[i] < minimum ? minimum : chord[i])
            : nearestInScaleAtOrAbove(chord[i], keyRoot, scale, minimum);
        if (pitch <= 127) {
            result.push_back(pitch);
            minimum = pitch + 1;
        }
    }
    return result;
}

}
