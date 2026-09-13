#pragma once
#include "engine/MidiLooper.h"
#include <ArduinoJson.h>

namespace midibrain {
class LoopCodec {
public:
    static void encode(const MidiLooper& loop, JsonDocument& document);
    static bool decode(const JsonDocument& document, MidiLooper& loop);
};
}
