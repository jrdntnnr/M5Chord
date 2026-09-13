#pragma once
#ifdef ARDUINO
#include <ArduinoJson.h>
#include <SD.h>
#include <cstdio>
namespace midibrain {
inline bool readJsonFile(const char* path, JsonDocument& doc, std::size_t limit = 65536) {
    char backup[128]{};
    std::snprintf(backup,sizeof(backup),"%s.bak",path);
    File file = SD.open(path, FILE_READ);
    if (!file) file = SD.open(backup, FILE_READ);
    if (!file || file.size() > limit) return false;
    const auto error = deserializeJson(doc,file);
    file.close();
    return !error;
}
inline bool writeJsonFile(const char* path, const JsonDocument& doc) {
    if (doc.overflowed()) return false;
    char temporary[128]{};
    char backup[128]{};
    if (std::snprintf(temporary,sizeof(temporary),"%s.tmp",path) >= static_cast<int>(sizeof(temporary))) return false;
    std::snprintf(backup,sizeof(backup),"%s.bak",path);
    SD.remove(temporary);
    File file = SD.open(temporary,FILE_WRITE);
    if (!file) return false;
    const bool written = serializeJson(doc,file) == measureJson(doc);
    file.flush();
    file.close();
    if (!written) { SD.remove(temporary); return false; }
    if (SD.exists(path)) {
        SD.remove(backup);
        if (!SD.rename(path,backup)) { SD.remove(temporary); return false; }
    }
    if (!SD.rename(temporary,path)) {
        if (SD.exists(backup)) SD.rename(backup,path);
        return false;
    }
    return true;
}
}
#endif
