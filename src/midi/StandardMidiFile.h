#pragma once
#include "common/FixedList.h"
#include "midi/MidiEvent.h"
#include <cstddef>
#include <cstdint>
#include <array>

namespace midibrain {

class MidiFileReader {
public:
    virtual ~MidiFileReader() = default;
    virtual uint32_t size() const = 0;
    virtual bool read(uint32_t offset, uint8_t* data, std::size_t count) = 0;
};

struct MidiFileEvent {
    uint64_t at_us{0};
    MidiType type{MidiType::Unknown};
    uint8_t channel{0};
    uint8_t data1{0};
    uint8_t data2{0};
    uint16_t value14{0};
};

class MidiFileDecoder {
public:
    enum class Result { Event, Metadata, End, Error };
    bool begin(MidiFileReader& reader);
    Result next(MidiFileReader& reader, MidiFileEvent& event);
    const char* error() const { return error_; }
    uint64_t duration() const { return now_; }
    uint16_t channels() const { return channels_; }
    uint16_t tracks() const { return count_; }
    uint32_t skippedSysex() const { return skipped_sysex_; }
    uint8_t progress() const;
private:
    struct Track {
        uint32_t position{0}, start{0}, end{0};
        uint64_t tick{0};
        uint8_t running{0};
        bool done{false};
    };
    bool byte(MidiFileReader& reader, Track& track, uint8_t& value);
    bool variable(MidiFileReader& reader, Track& track, uint32_t& value);
    bool fail(const char* reason);
    std::array<Track, 32> cursors_{};
    uint64_t tick_{0}, now_{0}, remainder_{0};
    uint32_t tempo_{500000}, skipped_sysex_{0};
    uint16_t count_{0}, division_{0}, channels_{0};
    const char* error_{"NO MIDI FILE"};
};

struct StandardMidiFile {
    static constexpr std::size_t Capacity = 2048;
    static constexpr uint32_t MaximumBytes = 1024 * 1024;
    static constexpr uint64_t MaximumDurationUs = 24ULL * 60 * 60 * 1000000;
    FixedList<MidiFileEvent, Capacity> events{};
    uint64_t duration_us{0};
    uint16_t channels{0};
    uint16_t tracks{0};
    uint32_t skipped_sysex{0};
    const char* error{"NO MIDI FILE"};
    bool load(MidiFileReader& reader);
    bool valid() const { return error == nullptr; }
};

}
