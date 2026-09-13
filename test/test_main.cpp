#include <exception>
#include <iostream>

void testChords();
void testBleMidi();
void testLifecycle();
void testMapper();
void testPadLearner();
void testMidiSink();
void testParser();
void testPerformance();
void testRouter();
void testScales();
void testScheduler();
void testSettings();
void testVoicing();
void testExpansion();
void testLooper();
void testHarmonyDisplay();
void testKeyExtensions();
void testLiveKeyboard();
void testCompatibility();

int main() {
    try {
        testChords();
        testBleMidi();
        testLifecycle();
        testMapper();
        testPadLearner();
        testMidiSink();
        testParser();
        testPerformance();
        testRouter();
        testScales();
        testScheduler();
        testSettings();
        testVoicing();
        testExpansion();
        testLooper();
        testHarmonyDisplay();
        testKeyExtensions();
        testLiveKeyboard();
        testCompatibility();
        std::cout << "All tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
