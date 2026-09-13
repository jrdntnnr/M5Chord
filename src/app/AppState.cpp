#include "app/AppState.h"

namespace midibrain {

const char* performanceName(PerformanceMode mode) {
    switch (mode) {
        case PerformanceMode::Block: return "BLOCK";
        case PerformanceMode::Strum: return "STRUM";
        case PerformanceMode::StrumTwoOctaves: return "STRUM 2 OCT";
        case PerformanceMode::Slop: return "SLOP";
        case PerformanceMode::Arp: return "ARP";
        case PerformanceMode::ArpTwoOctaves: return "ARP 2 OCT";
        case PerformanceMode::Pattern: return "PATTERN";
        case PerformanceMode::Harp: return "HARP";
    }
    return "BLOCK";
}

const char* modeName(EngineMode mode) {
    switch (mode) {
        case EngineMode::Bypass: return "BYPASS";
        case EngineMode::Chord: return "CHORD";
        case EngineMode::Key: return "KEY";
    }
    return "BYPASS";
}

}
