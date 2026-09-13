#pragma once
#include "midi/StandardMidiFile.h"
#include "midi/MidiFileStream.h"
#include "scheduler/MidiScheduler.h"
#include <array>

namespace midibrain {
class MidiFilePlayer {
public:
    StandardMidiFile& file() { return file_; }
    const StandardMidiFile& file() const { return file_; }
    bool play(uint64_t nowUs, uint8_t channels);
    void stop();
    bool tick(uint64_t nowUs, MidiScheduler& scheduler);
    bool finished(uint64_t nowUs) const;
    bool playing() const { return playing_; }
    uint8_t usedChannels() const { return used_channels_; }
    const char* status() const { return status_; }
    void setName(const char* name);
    const char* name() const { return name_; }
    void attach(MidiFileStream* stream) { stream_ = stream; }
    bool buffering() const { return waiting_; }
    uint64_t elapsed(uint64_t nowUs) const { return playing_ && !waiting_ && nowUs >= origin_us_ ? std::min(nowUs - origin_us_, file_.duration_us) : elapsed_us_; }
private:
    struct Held { uint32_t generation{0}; uint8_t channel{0}; uint8_t note{0}; };
    StandardMidiFile file_{};
    std::array<Held, 256> held_{};
    std::array<int8_t, 16> map_{};
    std::size_t next_{0};
    uint64_t origin_us_{0};
    uint64_t elapsed_us_{0};
    MidiFileStream* stream_{nullptr};
    uint32_t token_{0};
    bool waiting_{false};
    uint8_t used_channels_{0};
    bool playing_{false};
    bool failed_{false};
    const char* status_{"NO MIDI FILE"};
    char name_[64]{};
};
}
