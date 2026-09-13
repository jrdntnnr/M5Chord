#pragma once
#include <cstddef>

namespace midibrain {

inline bool containsAscii(const char* text, const char* match) {
    if (!text || !match) return false;
    for (std::size_t i = 0; text[i]; ++i) {
        std::size_t j = 0;
        while (match[j] && text[i + j]) {
            char c = text[i + j];
            if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
            if (c != match[j]) break;
            ++j;
        }
        if (!match[j]) return true;
    }
    return false;
}

inline bool bleMidiCandidate(bool serviceAdvertised, const char* name) {
    return serviceAdvertised || containsAscii(name, "MIDI")
        || ((containsAscii(name, "SMK-37") || containsAscii(name, "SMK37")) && containsAscii(name, "BLE"));
}

}
