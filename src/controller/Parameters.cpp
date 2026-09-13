#include "controller/Parameters.h"
#include <cstdio>

namespace midibrain {
void formatParameter(std::size_t index, const AppState& state, char* output, std::size_t size) {
    const int value = parameterValue(index, state);
    const char* label = nullptr;
    const char* styles[]{"SIMPLE", "ADVANCED", "FREE", "LATCHED"};
    const char* rates[]{"1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32"};
    const char* roots[]{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const char* dirs[]{"UP", "DOWN", "UP/DOWN", "RANDOM"};
    const char* bass[]{"OFF", "ROOT", "LOWEST", "UNISON"};
    const char* expression[]{"SOURCE", "GENERATED", "OFF"};
    const char* inputs[]{"AUTO", "BLE", "USB", "DIN"};
    if (index == 0) label = modeName(state.mode);
    else if (index == 1) label = styles[value];
    else if (index == 2) label = value ? "STACK" : "MOMENTARY";
    else if (index == 3) label = value ? "RETRIGGER" : "ADD NOTE";
    else if (index == 4) label = roots[value];
    else if (index == 5) label = ScaleEngine{}.definition(state.harmonic.scale).name;
    else if (index == 8) label = performanceName(state.performance.mode);
    else if (index == 9) label = dirs[value];
    else if (index == 10) label = rates[value];
    else if (index == 22) label = bass[value];
    else if (index == 23) label = expression[value];
    else if (index == 27 && !value) label = "FREE";
    else if (index == 28) label = value ? rates[value - 1] : "OFF";
    else if (index == 32) label = value ? "SOURCE VELOCITY" : "FIXED 100";
    else if (index == 33) label = inputs[value];
    else if (index == 15 || (index >= 19 && index <= 21) || index == 30) label = value ? "ON" : "OFF";
    if (label) std::snprintf(output, size, "%s", label);
    else std::snprintf(output, size, "%d", value);
}
}
