#include "engine/VoicingEngine.h"

namespace midibrain {

NoteList VoicingEngine::apply(const NoteList& chord, int step) const {
    NoteList result = chord;
    result.sort([](uint8_t left, uint8_t right) { return left < right; });
    while (step > 0 && !result.empty()) {
        const int raised = static_cast<int>(result[0]) + 12;
        if (raised > 127) {
            break;
        }
        result[0] = static_cast<uint8_t>(raised);
        result.sort([](uint8_t left, uint8_t right) { return left < right; });
        --step;
    }
    while (step < 0 && !result.empty()) {
        const std::size_t highest = result.size() - 1;
        const int lowered = static_cast<int>(result[highest]) - 12;
        if (lowered < 0) {
            break;
        }
        result[highest] = static_cast<uint8_t>(lowered);
        result.sort([](uint8_t left, uint8_t right) { return left < right; });
        ++step;
    }
    return result;
}

NoteList VoicingEngine::expandOctaves(const NoteList& chord, uint8_t octaveCount) const {
    NoteList result;
    for (uint8_t octave = 0; octave < octaveCount; ++octave) {
        for (const uint8_t note : chord) {
            const int candidate = static_cast<int>(note) + 12 * octave;
            if (candidate <= 127) {
                result.push_back(static_cast<uint8_t>(candidate));
            }
        }
    }
    result.sort([](uint8_t left, uint8_t right) { return left < right; });
    return result;
}

}
