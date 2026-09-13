#pragma once

#include <cstdint>

namespace midibrain {

enum class MidiType : uint8_t {
    NoteOff,
    NoteOn,
    PolyAftertouch,
    ControlChange,
    ProgramChange,
    ChannelPressure,
    PitchBend,
    Clock,
    Start,
    Continue,
    Stop,
    SongPosition,
    SysEx,
    Unknown
};

struct MidiEvent {
    MidiType type{MidiType::Unknown};
    uint8_t channel{0};
    uint8_t data1{0};
    uint8_t data2{0};
    uint16_t value14{0};
    uint64_t timestamp_us{0};
    uint8_t cable{0};

    static MidiEvent noteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint64_t at = 0) {
        return {MidiType::NoteOn, channel, note, velocity, 0, at, 0};
    }

    static MidiEvent noteOff(uint8_t channel, uint8_t note, uint8_t velocity = 0, uint64_t at = 0) {
        return {MidiType::NoteOff, channel, note, velocity, 0, at, 0};
    }

    static MidiEvent cc(uint8_t channel, uint8_t number, uint8_t value, uint64_t at = 0) {
        return {MidiType::ControlChange, channel, number, value, 0, at, 0};
    }
};

struct VoiceId {
    uint8_t source_channel{0};
    uint8_t source_note{0};
    uint32_t generation{0};

    bool operator==(const VoiceId& other) const {
        return source_channel == other.source_channel && source_note == other.source_note && generation == other.generation;
    }

    bool operator!=(const VoiceId& other) const { return !(*this == other); }
};

enum class StreamId : uint8_t {
    Performance,
    Bass,
    RawChord,
    Loop,
    System
};

struct OutputNote {
    uint8_t channel{0};
    uint8_t note{0};

    bool operator==(const OutputNote& other) const {
        return channel == other.channel && note == other.note;
    }
};

}

