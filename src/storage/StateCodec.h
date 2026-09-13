#pragma once
#include "app/AppState.h"
#include <ArduinoJson.h>
namespace midibrain {
class StateCodec {
public:
    static void encode(const AppState& state, JsonDocument& document);
    static bool decode(const JsonDocument& document, AppState& state);
    static uint32_t fingerprint(const AppState& state);
};
}

