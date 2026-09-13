#pragma once

#include "common/FixedList.h"
#include "engine/MusicTypes.h"
#include "midi/MidiEvent.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace midibrain {

struct ActiveGeneratedNote {
    VoiceId owner{};
    StreamId stream{StreamId::Performance};
    uint8_t output_channel{0};
    uint8_t note{0};
    bool active{false};
};

class ActiveNoteRegistry {
public:
    static constexpr std::size_t Capacity = 256;
    bool activate(const VoiceId& owner, StreamId stream, uint8_t channel, uint8_t note, bool& shouldSend);
    bool deactivate(const VoiceId& owner, StreamId stream, uint8_t channel, uint8_t note, bool& shouldSend);
    bool owns(const VoiceId& owner, StreamId stream, uint8_t channel, uint8_t note) const;
    FixedList<OutputNote, Capacity> release(const VoiceId& owner);
    FixedList<OutputNote, Capacity> release(StreamId stream);
    FixedList<OutputNote, Capacity> releaseExcept(const VoiceId& owner, StreamId stream, uint8_t channel, const NoteList& desired);
    FixedList<OutputNote, Capacity> releaseAll();
    std::size_t size() const;
    bool empty() const;

private:
    bool pitchActive(uint8_t channel, uint8_t note) const;
    FixedList<OutputNote, Capacity> collectReleased();

    std::array<ActiveGeneratedNote, Capacity> notes_{};
    FixedList<OutputNote, Capacity> pending_releases_{};
};

}
