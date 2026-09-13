#pragma once

#include "app/App.h"
#include "hardware/InputKeys.h"
#include "storage/ProfileStore.h"
#include "transport/BleMidiSource.h"
#include "transport/UsbMidiSource.h"
#include "ui/HarmonyDisplay.h"
#include "ui/LiveKeyboard.h"
#include "common/DisplayView.h"

#include <cstdint>
#include <cstddef>

namespace midibrain {

class Ui {
public:
    void begin();
    void showBootStatus(const char* text);
    void update(uint64_t nowUs, const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles, const InputKeys& input);
    void showOverlay(const char* text, uint64_t nowUs);
    void nextView();
    DisplayView view() const { return view_; }
    void setView(DisplayView view) { view_ = storedDisplayView(static_cast<uint8_t>(view)); }
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
    void drawHelp(const InputKeys& input);
    void drawMidiPlayer(const App& app, const InputKeys& input, uint64_t nowUs);

    DisplayView view_{DisplayView::Keyboard};
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
