#include "TestSupport.h"
#include "storage/SettingsStore.h"
#include "storage/SdPaths.h"
#include "storage/StateCodec.h"
#include <cstdio>

using namespace midibrain;

void testSettings() {
    EXPECT(!std::strcmp(SdPaths::Root, "/M5Chord"));
    EXPECT(SdPaths::validProfile(SdPaths::DefaultProfile));
    EXPECT(SdPaths::validProfile("/M5Chord/controllers/custom.json"));
    EXPECT(!SdPaths::validProfile("/midi-brain/controllers/custom.json"));
    EXPECT(!SdPaths::validProfile("/M5Chord/controllers/../custom.json"));
    EXPECT(!SdPaths::validProfile("/M5Chord/controllers/"));
    EXPECT(!SdPaths::validProfile("/M5Chord/controllers-evil/custom.json"));
    EXPECT(!SdPaths::validProfile("/M5Chord/controllers/sub/custom.json"));
    EXPECT(!SdPaths::validProfile("/M5Chord/controllers/\\custom.json"));
    EXPECT(!SdPaths::validProfile(nullptr));
    char path[80]{};
    std::snprintf(path, sizeof(path), SdPaths::PresetPattern, 1U);
    EXPECT(!std::strcmp(path, "/M5Chord/presets/preset-01.json"));
    std::snprintf(path, sizeof(path), SdPaths::LoopPattern, 16U);
    EXPECT(!std::strcmp(path, "/M5Chord/loops/loop-16.json"));
    EXPECT(!std::strcmp(SdPaths::Diagnostics, "/M5Chord/logs/diagnostics.json"));
    SettingsStore store;
    AppState firstBoot;
    EXPECT(!store.load(firstBoot) && firstBoot.mode == EngineMode::Chord);
    EXPECT(store.loadView() == DisplayView::Keyboard);
    for (unsigned value = 0; value < 256; ++value) {
        EXPECT(storedDisplayView(value) == (value < 4 ? static_cast<DisplayView>(value) : DisplayView::Keyboard));
    }
    EXPECT(!store.saveView(static_cast<DisplayView>(4)));
    for (auto mode : {EngineMode::Bypass, EngineMode::Chord, EngineMode::Key}) {
        AppState saved; saved.mode = mode;
        JsonDocument document; StateCodec::encode(saved, document);
        AppState restored; restored.mode = EngineMode::Chord;
        EXPECT(StateCodec::decode(document, restored) && restored.mode == mode);
    }
    AppState state;
    const uint32_t initial = store.fingerprint(state);
    state.harmonic.extensions = ExtensionMajor7;
    EXPECT(store.fingerprint(state) == initial);
    state.harmonic.extension_stack = true;
    EXPECT(store.fingerprint(state) != initial);
}
