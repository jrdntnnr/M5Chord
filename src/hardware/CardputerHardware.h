#pragma once

#ifdef ARDUINO
#include <M5Unified.h>
#endif

namespace midibrain {

struct CardputerPins {
    static constexpr int SdClock = 40;
    static constexpr int SdMiso = 39;
    static constexpr int SdMosi = 14;
    static constexpr int SdSelect = 12;
    static constexpr int MidiRx = 1;
    static constexpr int MidiTx = 2;
};

enum class CardputerFamily { Unsupported, Classic, Adv };

constexpr const char* cardputerFamilyName(CardputerFamily family) {
    switch (family) {
        case CardputerFamily::Classic: return "1.0/1.1";
        case CardputerFamily::Adv: return "ADV";
        default: return "UNKNOWN";
    }
}

#ifdef ARDUINO
inline CardputerFamily detectedCardputerFamily() {
    switch (M5.getBoard()) {
        case m5::board_t::board_M5Cardputer: return CardputerFamily::Classic;
        case m5::board_t::board_M5CardputerADV: return CardputerFamily::Adv;
        default: return CardputerFamily::Unsupported;
    }
}
#endif

}
