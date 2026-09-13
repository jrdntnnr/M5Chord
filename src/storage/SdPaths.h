#pragma once
#include <cstring>

namespace midibrain::SdPaths {
inline constexpr char Root[] = "/M5Chord";
inline constexpr char Controllers[] = "/M5Chord/controllers";
inline constexpr char ControllerPrefix[] = "/M5Chord/controllers/";
inline constexpr char DefaultProfile[] = "/M5Chord/controllers/smk37.json";
inline constexpr char Presets[] = "/M5Chord/presets";
inline constexpr char Loops[] = "/M5Chord/loops";
inline constexpr char Logs[] = "/M5Chord/logs";
inline constexpr char PresetPattern[] = "/M5Chord/presets/preset-%02u.json";
inline constexpr char LoopPattern[] = "/M5Chord/loops/loop-%02u.json";
inline constexpr char Diagnostics[] = "/M5Chord/logs/diagnostics.json";

inline bool validProfile(const char* path) {
    constexpr auto prefixLength = sizeof(ControllerPrefix) - 1;
    return path && !std::strncmp(path, ControllerPrefix, prefixLength) && path[prefixLength]
        && !std::strstr(path, "..") && !std::strchr(path + prefixLength, '/') && !std::strchr(path, '\\');
}
}
