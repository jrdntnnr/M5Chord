#pragma once
#include <cstdint>

namespace midibrain {
enum class DisplayView : uint8_t { Chord, Notes, Keyboard, Geek };
constexpr DisplayView storedDisplayView(uint8_t value) {
    return value < 4 ? static_cast<DisplayView>(value) : DisplayView::Keyboard;
}
}
