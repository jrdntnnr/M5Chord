#pragma once

#include "app/App.h"
#include "hardware/InputKeys.h"
#include "storage/ProfileStore.h"
#include "transport/BleMidiSource.h"
#include "transport/UsbMidiSource.h"
#include "ui/HarmonyDisplay.h"
#include "ui/LiveKeyboard.h"

#include <cstdint>
#include <cstddef>

namespace midibrain {

enum class DisplayView : uint8_t {
    Chord,
    Notes,
    Keyboard,
    Geek
};

class Ui {
public:
    void begin();
    void showBootStatus(const char* text);
    void update(uint64_t nowUs, const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles, const InputKeys& input);
    void showOverlay(const char* text, uint64_t nowUs);
    void nextView();
#ifdef MIDIBRAIN_UI_PREVIEW
    bool savePreview(const char* path) const;
#endif

private:
    void drawChrome(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles);
    void drawPerformance(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles);
    void drawNotes(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles);
    void drawKeyboard(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles);
    void drawGeek(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles);
    void drawLearn(const InputKeys& input);
    void drawIndicators(const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles);
    void drawPads();
    void drawOptions(const InputKeys& input, uint64_t nowUs);

    DisplayView view_{DisplayView::Chord};
    HarmonyDisplay harmony_{};
    LiveKeyboard keyboard_{};
    const char* overlay_{nullptr};
    char overlay_text_[96]{};
    uint64_t overlay_until_us_{0};
    uint64_t next_frame_us_{0};
    uint32_t last_rx_{0};
    uint64_t rx_until_us_{0};
    bool rx_active_{false};
};

}
