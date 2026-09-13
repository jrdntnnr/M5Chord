cmake_minimum_required(VERSION 3.20)
project(M5Chord VERSION 1.1.0 LANGUAGES CXX)

set(MIDIBRAIN_LIBDEPS "${CMAKE_SOURCE_DIR}/.pio/libdeps/cardputer-universal")
if(NOT EXISTS "${MIDIBRAIN_LIBDEPS}/ArduinoJson/src/ArduinoJson.h")
    set(MIDIBRAIN_LIBDEPS "${CMAKE_SOURCE_DIR}/.pio/libdeps/cardputer-adv")
endif()
set(ARDUINOJSON_INCLUDE "${MIDIBRAIN_LIBDEPS}/ArduinoJson/src" CACHE PATH "Pinned ArduinoJson headers")
if(NOT EXISTS "${ARDUINOJSON_INCLUDE}/ArduinoJson.h")
    message(FATAL_ERROR "Install pinned dependencies with pio pkg install -e cardputer-universal or set ARDUINOJSON_INCLUDE")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(midi_brain_core
    src/app/App.cpp
    src/app/AppState.cpp
    src/controller/ControllerMapper.cpp
    src/controller/PadLearner.cpp
    src/controller/Parameters.cpp
    src/engine/ChordEngine.cpp
    src/engine/PerformanceEngine.cpp
    src/engine/MidiLooper.cpp
    src/engine/MidiFilePlayer.cpp
    src/midi/StandardMidiFile.cpp
    src/storage/MidiFileStore.cpp
    src/engine/ScaleEngine.cpp
    src/engine/VoicingEngine.cpp
    src/midi/ActiveNoteRegistry.cpp
    src/midi/BleMidiDecoder.cpp
    src/midi/MidiParser.cpp
    src/midi/MidiRouter.cpp
    src/scheduler/MidiScheduler.cpp
    src/storage/SettingsStore.cpp
    src/storage/ProfileCodec.cpp
    src/storage/StateCodec.cpp
    src/storage/LoopCodec.cpp
    src/storage/ProfileStore.cpp
    src/hardware/InputKeys.cpp
    src/transport/DinMidiSink.cpp
    src/ui/HarmonyDisplay.cpp
)

target_include_directories(midi_brain_core PUBLIC src)
target_include_directories(midi_brain_core SYSTEM PUBLIC "${ARDUINOJSON_INCLUDE}")
target_compile_options(midi_brain_core PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(midi_brain_tests
    test/test_main.cpp
    test/test_chords.cpp
    test/test_lifecycle.cpp
    test/test_mapper.cpp
    test/test_pad_learner.cpp
    test/test_ble_midi.cpp
    test/test_midi_sink.cpp
    test/test_parser.cpp
    test/test_performance.cpp
    test/test_router.cpp
    test/test_scales.cpp
    test/test_scheduler.cpp
    test/test_settings.cpp
    test/test_voicing.cpp
    test/test_expansion.cpp
    test/test_looper.cpp
    test/test_harmony_display.cpp
    test/test_key_extensions.cpp
    test/test_live_keyboard.cpp
    test/test_compatibility.cpp
    test/test_connections_help.cpp
    test/test_midi_file.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(midi_brain_tests PRIVATE midi_brain_core Threads::Threads)
target_compile_options(midi_brain_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_executable(midi_file_check tools/check_midi_file.cpp)
target_link_libraries(midi_file_check PRIVATE midi_brain_core)
target_compile_options(midi_file_check PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME midi_brain_tests COMMAND midi_brain_tests)

option(MIDIBRAIN_BUILD_UI_PREVIEW "Build the native framebuffer preview" OFF)
if(MIDIBRAIN_BUILD_UI_PREVIEW)
    enable_language(C)
    set(M5GFX_ROOT "${MIDIBRAIN_LIBDEPS}/M5GFX/src")
    find_program(SDL2_CONFIG sdl2-config REQUIRED)
    execute_process(COMMAND ${SDL2_CONFIG} --cflags OUTPUT_VARIABLE SDL2_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND ${SDL2_CONFIG} --libs OUTPUT_VARIABLE SDL2_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    separate_arguments(SDL2_FLAGS NATIVE_COMMAND "${SDL2_FLAGS}")
    separate_arguments(SDL2_LIBS NATIVE_COMMAND "${SDL2_LIBS}")
    file(GLOB M5GFX_PREVIEW_SOURCES CONFIGURE_DEPENDS
        "${M5GFX_ROOT}/lgfx/v1/*.cpp"
        "${M5GFX_ROOT}/lgfx/v1/misc/*.cpp"
        "${M5GFX_ROOT}/lgfx/v1/panel/Panel_Device.cpp"
        "${M5GFX_ROOT}/lgfx/v1/platforms/sdl/*.cpp"
        "${M5GFX_ROOT}/lgfx/utility/*.c"
        "${M5GFX_ROOT}/lgfx/Fonts/efont/*.c"
        "${M5GFX_ROOT}/lgfx/Fonts/IPA/*.c")
    add_library(midi_preview_gfx STATIC ${M5GFX_PREVIEW_SOURCES})
    target_include_directories(midi_preview_gfx SYSTEM PUBLIC "${M5GFX_ROOT}")
    target_compile_options(midi_preview_gfx PUBLIC ${SDL2_FLAGS})
    target_link_libraries(midi_preview_gfx PUBLIC ${SDL2_LIBS})
    add_executable(midi_ui_preview tools/ui_preview.cpp src/ui/Ui.cpp src/transport/BleMidiSource.cpp src/transport/UsbMidiSource.cpp)
    target_compile_definitions(midi_ui_preview PRIVATE MIDIBRAIN_UI_PREVIEW)
    target_link_libraries(midi_ui_preview PRIVATE midi_brain_core midi_preview_gfx)
endif()
