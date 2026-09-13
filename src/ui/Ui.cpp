#include "ui/Ui.h"
#include "common/AppInfo.h"
#include "hardware/CardputerHardware.h"

#ifdef ARDUINO
#include <M5Cardputer.h>
#elif defined(MIDIBRAIN_UI_PREVIEW)
#include <lgfx/v1/LGFX_Sprite.hpp>
using M5Canvas = lgfx::LGFX_Sprite;
using lgfx::top_left;
static struct { M5Canvas Display; } M5Cardputer;
#endif

#if defined(ARDUINO) || defined(MIDIBRAIN_UI_PREVIEW)
#define MIDIBRAIN_DISPLAY
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace midibrain {

#ifdef MIDIBRAIN_DISPLAY
namespace {
M5Canvas frame(&M5Cardputer.Display);
bool frameReady{false};
constexpr uint32_t background = 0x000000;
constexpr uint32_t panel = 0x002000;
constexpr uint32_t ink = 0xa0e080;
constexpr uint32_t muted = 0x60a040;
constexpr uint32_t quiet = 0x206000;
constexpr uint32_t accent = 0x80e080;
constexpr uint32_t selected = 0xe0a040;
constexpr uint32_t border = 0x004000;

void line(const char* value, int x, int y, int width, const lgfx::IFont* font = &fonts::Font2,
          uint32_t color = ink, uint32_t fill = background, bool centered = false, float size = 1) {
    char text[128]{};
    std::snprintf(text, sizeof(text), "%s", value ? value : "");
    frame.setFont(font);
    frame.setTextSize(size);
    frame.setTextDatum(top_left);
    frame.setTextColor(color, fill);
    if (frame.textWidth(text) > width) {
        std::size_t length = std::strlen(text);
        while (length && frame.textWidth(text) + frame.textWidth("...") > width) text[--length] = 0;
        std::snprintf(text + length, sizeof(text) - length, "...");
    }
    frame.setClipRect(x, y, width, std::min(135 - y, frame.fontHeight() + 1));
    frame.drawString(text, x + (centered ? std::max(0, (width - frame.textWidth(text)) / 2) : 0), y);
    frame.clearClipRect();
    frame.setTextSize(1);
}

void paragraph(const char* value, int x, int y, int width, int rows, uint32_t color = muted) {
    frame.setFont(&fonts::Font0);
    frame.setTextSize(1);
    const char* cursor = value;
    for (int row = 0; row < rows && *cursor; ++row) {
        char part[128]{};
        std::size_t size = 0;
        std::size_t lastSpace = 0;
        while (cursor[size] && size < sizeof(part) - 1) {
            part[size] = cursor[size];
            part[size + 1] = 0;
            if (frame.textWidth(part) > width) { part[size] = 0; break; }
            if (cursor[size] == ' ') lastSpace = size;
            ++size;
        }
        if (cursor[size] && row + 1 < rows && lastSpace) { size = lastSpace; part[size] = 0; }
        if (cursor[size] && row + 1 == rows) { line(cursor, x, y + row * 11, width, &fonts::Font0, color); break; }
        line(part, x, y + row * 11, width, &fonts::Font0, color);
        cursor += size;
        while (*cursor == ' ') ++cursor;
        if (!size) break;
    }
}

void displayText(const char* text, int y, int height, uint32_t color = ink) {
    const lgfx::IFont* font = &fonts::Orbitron_Light_32;
    frame.setFont(font);
    const float scale = std::min(1.5f, std::min(224.0f / std::max(1, frame.textWidth(text)), static_cast<float>(height) / frame.fontHeight()));
    if (scale >= 1) {
        line(text, 8, y + std::max(0, (height - static_cast<int>(frame.fontHeight() * scale)) / 2), 224, font, color, background, true, scale);
        return;
    }
    font = &fonts::Orbitron_Light_24;
    frame.setFont(font);
    if (frame.textWidth(text) <= 224) {
        line(text, 8, y + std::max(0, (height - frame.fontHeight()) / 2), 224, font, color, background, true);
        return;
    }
    const bool chord = text[0] >= 'A' && text[0] <= 'G';
    if (chord) {
        char root[3]{text[0], text[1] == '#' ? '#' : '\0', '\0'};
        frame.setFont(&fonts::Orbitron_Light_32);
        const float rootSize = std::min(1.25f, 88.0f / std::max(1, frame.textWidth(root)));
        line(root, 8, y, 88, &fonts::Orbitron_Light_32, color, background, false, rootSize);
        line(text + std::strlen(root), 104, y + 14, 128, &fonts::Font2, color);
    } else line(text, 8, y + 12, 224, &fonts::Font2, color, background, true);
}

const char* shortPerformance(PerformanceMode mode) {
    constexpr const char* names[]{"BLOCK", "STRUM", "STRUM2", "SLOP", "ARP", "ARP2", "PATTERN", "HARP"};
    return names[static_cast<unsigned>(mode)];
}

void rule(int y) { frame.drawFastHLine(8, y, 224, border); }
}
#endif

void Ui::begin() {
#ifdef MIDIBRAIN_DISPLAY
#ifdef MIDIBRAIN_UI_PREVIEW
    M5Cardputer.Display.setColorDepth(16);
    M5Cardputer.Display.createSprite(240, 135);
#else
    M5Cardputer.Display.setRotation(1);
#endif
    frame.setColorDepth(8);
    frameReady = frame.createSprite(240, 135) != nullptr;
    if (!frameReady) {
        M5Cardputer.Display.fillScreen(background);
        M5Cardputer.Display.setFont(&fonts::Font2);
        M5Cardputer.Display.setTextColor(ink, background);
        M5Cardputer.Display.drawString("DISPLAY MEMORY ERROR", 6, 50);
        return;
    }
    frame.setTextWrap(false, false);
    frame.fillScreen(background);
#endif
}

void Ui::showBootStatus(const char* text) {
#ifdef MIDIBRAIN_DISPLAY
    if (!frameReady) return;
    frame.fillScreen(background);
    char version[48]{};
    std::snprintf(version, sizeof(version), "CONTROLLER / V%s", AppVersion);
    line(version, 8, 8, 224, &fonts::Font0, muted);
    rule(22);
    displayText(AppName, 35, 42);
    line(text, 8, 101, 224, &fonts::Font2, muted, background, true);
    frame.pushSprite(0, 0);
#else
    static_cast<void>(text);
#endif
}

void Ui::showOverlay(const char* text, uint64_t nowUs) {
    std::snprintf(overlay_text_, sizeof(overlay_text_), "%s", text ? text : "");
    overlay_ = overlay_text_;
    overlay_until_us_ = nowUs + 1500000ULL;
    next_frame_us_ = 0;
}

void Ui::nextView() {
    view_ = static_cast<DisplayView>((static_cast<uint8_t>(view_) + 1) % 4);
    next_frame_us_ = 0;
}

void Ui::drawIndicators(const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles) {
#ifdef MIDIBRAIN_DISPLAY
    const auto state = ble.diagnostics().state;
    const bool pending = ble.scanning() || state == BleMidiState::Connecting || state == BleMidiState::Subscribing;
    frame.fillRect(126, 10, 3, 3, rx_active_ ? accent : border);
    line("USB", 143, 8, 23, &fonts::Font0, usb.connected() ? accent : quiet);
    line("BT", 181, 8, 16, &fonts::Font0, ble.connected() ? accent : pending ? selected : quiet);
    line("SD", 219, 8, 14, &fonts::Font0, profiles.available() ? accent : quiet);
#else
    static_cast<void>(usb); static_cast<void>(ble); static_cast<void>(profiles);
#endif
}

void Ui::drawChrome(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles) {
#ifdef MIDIBRAIN_DISPLAY
    const auto& state = app.state();
    frame.fillScreen(background);
    line(modeName(state.mode), 8, 3, 64);
    char text[80]{};
    std::snprintf(text, sizeof(text), "L%u", state.routing.performance_channel + 1);
    line(text, 85, 3, 32, &fonts::Font2, muted);
    drawIndicators(usb, ble, profiles);
    rule(22);
    std::snprintf(text, sizeof(text), "%s", harmony_.scaleLabel());
    for (auto& c : text) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    line(text, 8, 82, 153, &fonts::Font0, harmony_.keyActive() ? muted : quiet);
    const char* status = state.mode == EngineMode::Bypass ? "DIRECT" : state.mode == EngineMode::Key && !app.heldQualities() ? "AUTO"
        : state.harmonic.harmonic_quantize ? "SNAP ON" : state.mode == EngineMode::Key ? "MANUAL" : "SNAP OFF";
    line(status, 179, 82, 54, &fonts::Font0, harmony_.keyActive() ? muted : selected);
    drawPads();
    rule(117);
    std::snprintf(text, sizeof(text), "%s %u", shortPerformance(state.performance.mode), state.performance.bpm);
    line(text, 8, 124, 76, &fonts::Font0, muted);
    line(state.harmonic.extension_stack ? "STACK" : "HOLD", 93, 124, 35, &fonts::Font0, state.harmonic.extension_stack ? selected : muted);
    constexpr const char* styles[]{"SIMPLE", "ADV", "FREE", "LATCH"};
    line(styles[static_cast<unsigned>(state.harmonic.play_style)], 144, 124, 42, &fonts::Font0, muted);
    constexpr const char* loops[]{"", "REC", "LOOP", "DUB"};
    line(loops[static_cast<unsigned>(app.looper().mode())], 207, 124, 26, &fonts::Font0, selected);
#else
    static_cast<void>(app); static_cast<void>(usb); static_cast<void>(ble); static_cast<void>(profiles);
#endif
}

void Ui::drawPerformance(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles) {
#ifdef MIDIBRAIN_DISPLAY
    drawChrome(app, usb, ble, profiles);
    if (harmony_.chordVisible()) displayText(harmony_.chordLabel(), 25, 53, harmony_.brightness() < 128 ? muted : ink);
    else if (std::strcmp(harmony_.chordLabel(), "LOOP") == 0) displayText("LOOP", 28, 48, muted);
    else frame.fillRect(104, 52, 32, 2, quiet);
#else
    static_cast<void>(app); static_cast<void>(usb); static_cast<void>(ble); static_cast<void>(profiles);
#endif
}

void Ui::drawNotes(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles) {
#ifdef MIDIBRAIN_DISPLAY
    drawChrome(app, usb, ble, profiles);
    if (harmony_.chordVisible()) displayText(harmony_.chordLabel(), 27, 36);
    char text[128]{};
    std::size_t used = 0;
    constexpr const char* names[]{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for (const uint8_t note : harmony_.notes()) {
        const auto written = std::snprintf(text + used, sizeof(text) - used, "%s%d ", names[note % 12], note / 12 - 1);
        if (written < 0 || static_cast<std::size_t>(written) >= sizeof(text) - used) break;
        used += written;
    }
    line(text, 8, 68, 224, &fonts::Font0, ink, background, true);
#else
    static_cast<void>(app); static_cast<void>(usb); static_cast<void>(ble); static_cast<void>(profiles);
#endif
}

void Ui::drawKeyboard(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles) {
#ifdef MIDIBRAIN_DISPLAY
    drawChrome(app, usb, ble, profiles);
    char label[32]{};
    std::snprintf(label, sizeof(label), "LIVE C%d", keyboard_.base() / 12 - 1);
    line(label, 8, 28, 82, &fonts::Font0, muted);
    line(harmony_.chordVisible() ? harmony_.chordLabel() : "", 96, 27, 136, &fonts::Font0, ink);
    constexpr uint8_t whites[]{0, 2, 4, 5, 7, 9, 11};
    constexpr uint8_t blacks[]{1, 3, 6, 8, 10};
    constexpr uint8_t positions[]{1, 2, 4, 5, 6};
    const unsigned octaves = keyboard_.span() / 12;
    const unsigned count = octaves * 7;
    const auto edge = [count](unsigned index) { return 8 + index * 224 / count; };
    for (unsigned octave = 0; octave < octaves; ++octave) {
        for (unsigned i = 0; i < 7; ++i) {
            const auto note = keyboard_.base() + octave * 12 + whites[i];
            const int x = edge(octave * 7 + i);
            const int width = edge(octave * 7 + i + 1) - x - 1;
            frame.fillRect(x, 40, width, 36, keyboard_.gate(note) ? accent : panel);
            frame.drawRect(x, 40, width, 36, keyboard_.gate(note) ? accent : muted);
            if (keyboard_.attack(note)) frame.fillRect(x + 2, 69, std::max(1, width - 4), 4, selected);
        }
        for (unsigned i = 0; i < 5; ++i) {
            const auto note = keyboard_.base() + octave * 12 + blacks[i];
            const int width = std::max(4U, 112 / count);
            const int x = edge(octave * 7 + positions[i]) - width / 2;
            frame.fillRect(x, 40, width, 22, keyboard_.gate(note) ? accent : background);
            frame.drawRect(x, 40, width, 22, keyboard_.gate(note) ? accent : muted);
            if (keyboard_.attack(note)) frame.fillRect(x + 1, 57, std::max(1, width - 2), 3, selected);
        }
    }
#else
    static_cast<void>(app); static_cast<void>(usb); static_cast<void>(ble); static_cast<void>(profiles);
#endif
}

void Ui::drawGeek(const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles) {
#ifdef MIDIBRAIN_DISPLAY
    drawChrome(app, usb, ble, profiles);
    char text[96]{};
    std::snprintf(text, sizeof(text), "RX %lu  TX %lu  VOICE %u", static_cast<unsigned long>(app.state().stats.midi_events_rx), static_cast<unsigned long>(app.state().stats.midi_events_tx), static_cast<unsigned>(app.activeNotes().size()));
    line(text, 8, 30, 224, &fonts::Font0);
    std::snprintf(text, sizeof(text), "QUEUE %u  LATE %luus  DROP %lu", static_cast<unsigned>(app.scheduler().size()), static_cast<unsigned long>(app.scheduler().stats().max_late_us), static_cast<unsigned long>(app.state().stats.midi_events_dropped));
    line(text, 8, 42, 224, &fonts::Font0);
    const char* board = "SIM";
#ifdef ARDUINO
    board = cardputerFamilyName(detectedCardputerFamily());
#endif
    std::snprintf(text, sizeof(text), "%s USB %04X:%04X EP%02X", board, usb.diagnostics().vid, usb.diagnostics().pid, usb.diagnostics().endpoint_address);
    line(text, 8, 54, 224, &fonts::Font0, muted);
    std::snprintf(text, sizeof(text), "BT %s N%lu E%lu", BleMidiSource::stateName(ble.diagnostics().state), static_cast<unsigned long>(ble.diagnostics().notifications_received), static_cast<unsigned long>(ble.diagnostics().events_received));
    line(text, 8, 66, 224, &fonts::Font0, muted);
#else
    static_cast<void>(app); static_cast<void>(usb); static_cast<void>(ble); static_cast<void>(profiles);
#endif
}

void Ui::drawPads() {
#ifdef MIDIBRAIN_DISPLAY
    constexpr const char* labels[]{"DIM", "MIN", "MAJ", "SUS", "6", "m7", "M7", "9"};
    for (unsigned i = 0; i < 8; ++i) {
        const auto state = i < 4 ? harmony_.quality(i) : harmony_.extension(i - 4);
        const bool held = state == PadDisplayState::Held;
        const bool chosen = i < 4 ? harmony_.qualitySelected(i) : held || state == PadDisplayState::Selected;
        const uint32_t fill = held ? accent : state == PadDisplayState::Selected ? selected : background;
        const uint32_t color = held || state == PadDisplayState::Selected ? background : state == PadDisplayState::Disabled ? quiet : muted;
        const int x = 8 + i * 28;
        if (held || chosen) frame.fillRect(x, 94, 26, 19, fill);
        line(labels[i], x, 100, 26, &fonts::Font0, color, fill, true);
        if (chosen) frame.fillRect(x + 3, 112, 20, 1, ink);
        if (held) frame.fillRect(x + 23, 96, 2, 2, background);
    }
#endif
}

void Ui::drawLearn(const InputKeys& input) {
#ifdef MIDIBRAIN_DISPLAY
    frame.fillScreen(background);
    line("PAD / SETUP", 8, 3, 156);
    char step[16]{};
    std::snprintf(step, sizeof(step), "%u/%u", std::min<unsigned>(input.learnStep() + 1, PadLearner::MappingCount), static_cast<unsigned>(PadLearner::MappingCount));
    line(step, 192, 3, 40, &fonts::Font2, muted);
    rule(22);
    constexpr const char* names[]{"DIM", "MIN", "MAJ", "SUS", "ADD 6", "MIN 7", "MAJ 7", "ADD 9", "LAYER"};
    const bool wizard = std::strcmp(input.learnActionName(), "MIDI LEARN") != 0;
    displayText(wizard ? names[std::min<unsigned>(input.learnStep(), PadLearner::MappingCount - 1)] : "CONTROL", 34, 43);
    paragraph(input.learnStatus(), 8, 91, 224, 2);
    rule(117);
    line("RELEASE BETWEEN PADS       TAB EXIT", 8, 124, 224, &fonts::Font0, muted);
#else
    static_cast<void>(input);
#endif
}

void Ui::drawOptions(const InputKeys& input, uint64_t nowUs) {
#ifdef MIDIBRAIN_DISPLAY
    char title[64]{};
    char value[96]{};
    input.menuText(title, sizeof(title), value, sizeof(value));
    frame.fillScreen(background);
    line("OPTIONS", 8, 3, 138);
    char position[24]{};
    std::snprintf(position, sizeof(position), "%02u / %02u", static_cast<unsigned>(input.menuIndex() + 1), static_cast<unsigned>(input.menuCount()));
    line(position, 174, 8, 58, &fonts::Font0, muted);
    rule(22);
    const unsigned progress = 224 * (input.menuIndex() + 1) / input.menuCount();
    frame.drawFastHLine(8, 22, progress, accent);
    line(title, 8, 28, 224, &fonts::Font2, muted);
    frame.setFont(&fonts::Orbitron_Light_24);
    frame.setTextSize(1);
    const float valueSize = std::min(1.0f, 214.0f / std::max(1, frame.textWidth(value)));
    if (valueSize >= 0.75f) line(value, 18, 49, 214, &fonts::Orbitron_Light_24, ink, background, false, valueSize);
    else {
        frame.setFont(&fonts::Font2);
        if (frame.textWidth(value) <= 214) line(value, 18, 54, 214, &fonts::Font2);
        else paragraph(value, 18, 53, 214, 2, ink);
    }
    frame.fillRect(8, 53, 3, 21, accent);
    if (overlay_ && nowUs < overlay_until_us_) paragraph(overlay_, 8, 86, 224, 2, selected);
    else paragraph(input.menuHelp(), 8, 86, 224, 2);
    rule(117);
    line(";. MOVE  ,/ SET  ENTER OK  TAB EXIT", 8, 124, 224, &fonts::Font0, muted);
#else
    static_cast<void>(input); static_cast<void>(nowUs);
#endif
}

void Ui::update(uint64_t nowUs, const App& app, const UsbMidiSource& usb, const BleMidiSource& ble, const ProfileStore& profiles, const InputKeys& input) {
#ifdef MIDIBRAIN_DISPLAY
    if (!frameReady) return;
#endif
    if (nowUs < next_frame_us_) return;
    next_frame_us_ = nowUs + (view_ == DisplayView::Keyboard && !input.menuOpen() ? 16667ULL : 33333ULL);
    harmony_.update(app, nowUs);
    keyboard_.update(app, nowUs);
    if (app.state().stats.midi_events_rx != last_rx_) rx_until_us_ = nowUs + 120000;
    last_rx_ = app.state().stats.midi_events_rx;
    rx_active_ = nowUs < rx_until_us_;
    if (nowUs >= overlay_until_us_) overlay_ = nullptr;
#ifdef MIDIBRAIN_DISPLAY
    if (app.keyLearning()) {
        frame.fillScreen(background);
        line("KEY / LEARN", 8, 3, 224);
        rule(22);
        displayText("ROOT", 34, 43);
        line("PLAY A KEYBOARD NOTE", 8, 92, 224, &fonts::Font2, muted);
        rule(117);
        line("FN + ESC CANCEL", 8, 124, 224, &fonts::Font0, muted);
        frame.pushSprite(0, 0);
        return;
    }
#endif
    if (input.learning()) drawLearn(input);
    else if (input.menuOpen()) drawOptions(input, nowUs);
    else {
        if (view_ == DisplayView::Chord) drawPerformance(app, usb, ble, profiles);
        else if (view_ == DisplayView::Notes) drawNotes(app, usb, ble, profiles);
        else if (view_ == DisplayView::Keyboard) drawKeyboard(app, usb, ble, profiles);
        else drawGeek(app, usb, ble, profiles);
#ifdef MIDIBRAIN_DISPLAY
        if (overlay_) {
            frame.fillRect(0, 121, 240, 14, background);
            line(overlay_, 8, 124, 224, &fonts::Font0, selected);
        }
#endif
    }
#ifdef MIDIBRAIN_DISPLAY
    frame.pushSprite(0, 0);
#endif
}

#ifdef MIDIBRAIN_UI_PREVIEW
bool Ui::savePreview(const char* path) const {
    auto* output = std::fopen(path, "wb");
    if (!output) return false;
    std::fprintf(output, "P6\n240 135\n255\n");
    bool success = true;
    for (int y = 0; y < 135; ++y) for (int x = 0; x < 240; ++x) {
        const auto color = M5Cardputer.Display.readPixelRGB(x, y);
        const uint8_t pixel[]{color.R8(), color.G8(), color.B8()};
        success = std::fwrite(pixel, 1, sizeof(pixel), output) == sizeof(pixel) && success;
    }
    return std::fclose(output) == 0 && success;
}
#endif

}
