#include "TestSupport.h"
#include "storage/SettingsStore.h"

using namespace midibrain;

void testSettings() {
    SettingsStore store;
    AppState state;
    const uint32_t initial = store.fingerprint(state);
    state.harmonic.extensions = ExtensionMajor7;
    EXPECT(store.fingerprint(state) == initial);
    state.harmonic.extension_stack = true;
    EXPECT(store.fingerprint(state) != initial);
}
