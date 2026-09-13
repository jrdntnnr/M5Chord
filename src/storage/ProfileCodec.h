#pragma once
#include "controller/ControllerMapper.h"
#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace midibrain {
struct ControllerProfile {
    ControllerMapper mapper{};
    char name[65]{"Generic"};
    char manufacturer[65]{};
    char product[65]{};
    int32_t vid{-1};
    int32_t pid{-1};
    int8_t input_channel{-1};
    uint8_t low{0};
    uint8_t high{127};
};
class ProfileCodec {
public:
    static bool decode(const JsonDocument& document, ControllerProfile& output, const char*& error);
    static void encode(const ControllerProfile& profile, JsonDocument& document);
    static int match(const ControllerProfile& profile, uint16_t vid, uint16_t pid, const char* manufacturer, const char* product);
};
}

