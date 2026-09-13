#pragma once

#include "app/AppState.h"
#include "common/FixedList.h"
#include "controller/ControllerMapper.h"
#include "engine/ChordEngine.h"
#include "engine/PerformanceEngine.h"
#include "engine/ScaleEngine.h"
#include "engine/VoicingEngine.h"
#include "engine/MidiLooper.h"
#include "engine/MidiFilePlayer.h"
#include "midi/ActiveNoteRegistry.h"
#include "midi/MidiEvent.h"
#include "midi/MidiOutputActivity.h"
#include "midi/MidiRouter.h"
#include "scheduler/MidiScheduler.h"
#include "transport/MidiSink.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace midibrain {

class App {
public:
    explicit App(MidiSink& sink);
    void receive(const MidiEvent& event);
    void tick(uint64_t nowUs);
    void apply(const ActionEvent& action, uint64_t nowUs);
    void panic(uint64_t nowUs);
    void onUsbDisconnected(uint64_t nowUs);
    const AppState& state() const;
    AppState& state();
    const ActiveNoteRegistry& activeNotes() const;
    const MidiScheduler& scheduler() const;
    const NoteList& lastChord() const;
    uint8_t lastRoot() const;
    MidiLooper& looper();
    const MidiLooper& looper() const;
    void stopLoop(uint64_t nowUs);
    MidiFilePlayer& midiFile() { return midi_file_; }
    const MidiFilePlayer& midiFile() const { return midi_file_; }
    void stopMidiFile(uint64_t nowUs);
    bool keyLearning() const;
    bool captureKeySelection(const MidiEvent& event);
    bool idle() const;
    uint64_t lastActivity() const;
    bool hasHeldRoots() const;
    uint8_t heldQualities() const;
    const MidiOutputActivity& outputActivity() const { return output_activity_; }

private:
    struct SourceVoice {
        bool active{false};
        VoiceId id{};
        uint8_t velocity{0};
        NoteList notes{};
        uint64_t next_step_us{0};
        uint32_t step{0};
        bool chord{true};
        bool bypass{false};
        ChordQuality quality{ChordQuality::Major};
        uint8_t extensions{0};
    };

    SourceVoice* allocateVoice(const MidiEvent& event);
    void startVoice(const MidiEvent& event);
    void releaseVoices(uint8_t channel, uint8_t note, uint64_t nowUs);
    void releaseVoice(SourceVoice& voice, uint64_t nowUs);
    void rebuildHeld(uint64_t nowUs);
    NoteList buildNotes(uint8_t root, ChordQuality quality, uint8_t extensions) const;
    void scheduleVoice(SourceVoice& voice, uint64_t startUs);
    void scheduleRaw(const SourceVoice& voice, uint64_t startUs);
    void scheduleBass(const SourceVoice& voice, uint64_t startUs);
    void routeExpression(const MidiEvent& event);
    void dispatch(const ScheduledMidiEvent& scheduled);
    void send(const MidiEvent& event);
    void sendReleased(const FixedList<OutputNote, ActiveNoteRegistry::Capacity>& notes, uint64_t nowUs);
    void reconcileStream(const SourceVoice& voice, StreamId stream, uint8_t channel, const NoteList& desired, uint64_t nowUs);
    NoteList bassNotes(const SourceVoice& voice) const;
    void setPerformance(PerformanceMode mode, uint64_t nowUs);
    bool acceptsRoot(const MidiEvent& event) const;
    void changeRouting(uint8_t& channel, int value, uint64_t nowUs);
    void rebuildHarmony(uint64_t nowUs, bool qualityChange = false);

    MidiSink& sink_;
    MidiOutputActivity output_activity_{};
    AppState state_{};
    ChordEngine chords_{};
    ScaleEngine scales_{};
    VoicingEngine voicing_{};
    PerformanceEngine performance_{};
    MidiScheduler scheduler_{};
    ActiveNoteRegistry active_notes_{};
    std::array<SourceVoice, 16> voices_{};
    uint32_t generation_{0};
    NoteList last_chord_{};
    uint8_t last_root_{60};
    uint64_t last_tick_us_{0};
    uint64_t next_clock_us_{0};
    std::array<uint64_t, 4> tap_times_{};
    uint8_t tap_count_{0};
    MidiLooper looper_{};
    MidiFilePlayer midi_file_{};
    bool playback_dispatch_{false};
    bool key_learning_{false};
    uint8_t learned_key_channel_{0};
    uint8_t learned_key_note_{0};
    bool key_release_pending_{false};
    uint8_t quality_held_{0};
    std::array<SemanticAction, 256> held_controls_{};
    uint64_t last_activity_us_{0};
};

}
