#include "midi/StandardMidiFile.h"
#include "midi/MidiParser.h"
#include <array>
#include <cstring>

namespace midibrain {
namespace {
uint32_t big32(const uint8_t* p) { return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3]; }
uint16_t big16(const uint8_t* p) { return (uint16_t(p[0]) << 8) | p[1]; }
}
bool MidiFileDecoder::byte(MidiFileReader& reader, Track& track, uint8_t& value) {
    if (track.position >= track.end || !reader.read(track.position, &value, 1)) return false;
    ++track.position;
    return true;
}
bool MidiFileDecoder::variable(MidiFileReader& reader, Track& track, uint32_t& value) {
    value = 0;
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t b;
        if (!byte(reader, track, b)) return false;
        value = (value << 7) | (b & 127);
        if (!(b & 128)) return true;
    }
    return false;
}

bool MidiFileDecoder::begin(MidiFileReader& reader) {
    cursors_.fill({}); tick_ = 0; now_ = 0; remainder_ = 0; tempo_ = 500000;
    count_ = 0; division_ = 0; channels_ = 0; skipped_sysex_ = 0;
    error_ = "INVALID MIDI FILE";

    uint8_t header[14]{};
    if (reader.size() > StandardMidiFile::MaximumBytes) return fail("FILE EXCEEDS 1 MB");
    if (reader.size() < sizeof(header) || !reader.read(0, header, sizeof(header)) || std::memcmp(header, "MThd", 4)) return false;
    const uint32_t headerSize = big32(header + 4);
    const uint16_t format = big16(header + 8), count = big16(header + 10), division = big16(header + 12);
    if (headerSize < 6 || headerSize > reader.size() - 8) return false;
    if (format > 1) return fail("USE MIDI FORMAT 0 OR 1");
    if (!division || (division & 0x8000)) return fail("USE PPQN TIMING");
    if (!count || count > 32 || (format == 0 && count != 1)) return fail("TRACK LIMIT IS 32");
    uint32_t position = 8 + headerSize;
    for (unsigned i = 0; i < count; ++i) {
        uint8_t chunk[8]{};
        if (position > reader.size() || reader.size() - position < 8 || !reader.read(position, chunk, 8) || std::memcmp(chunk, "MTrk", 4)) return false;
        const uint32_t size = big32(chunk + 4);
        position += 8;
        if (size > reader.size() - position || !size) return false;
        auto& track = cursors_[i];
        track.position = position; track.start = position; track.end = position + size;
        uint32_t delta;
        if (!variable(reader, track, delta)) return false;
        track.tick = delta;
        position += size;
    }
    if (position != reader.size()) return false;
    count_ = count; division_ = division; error_ = nullptr;
    return true;
}
bool MidiFileDecoder::fail(const char* reason) { error_ = reason; return false; }
uint8_t MidiFileDecoder::progress() const {
    uint32_t consumed = 0, total = 0;
    for (unsigned i = 0; i < count_; ++i) {
        consumed += cursors_[i].position - cursors_[i].start;
        total += cursors_[i].end - cursors_[i].start;
    }
    return total ? static_cast<uint8_t>(uint64_t(consumed) * 100 / total) : 0;
}
MidiFileDecoder::Result MidiFileDecoder::next(MidiFileReader& reader, MidiFileEvent& output) {
    if (error_) return Result::Error;
    const auto fail = [this](const char* reason) { error_ = reason; return Result::Error; };
    MidiParser parser;
    const auto count = count_;
    const auto division = division_;
    Result result = Result::Metadata;
    Track* selected = nullptr;
    for (unsigned i = 0; i < count; ++i)
        if (!cursors_[i].done && (!selected || cursors_[i].tick < selected->tick)) selected = &cursors_[i];
    if (!selected) return channels_ ? Result::End : fail("NO NOTES IN FILE");
    const uint64_t delta = selected->tick - tick_;
    if (delta > StandardMidiFile::MaximumDurationUs * division / tempo_) return fail("FILE DURATION TOO LONG");
    const uint64_t scaled = delta * tempo_ + remainder_;
    now_ += scaled / division; remainder_ = scaled % division; tick_ = selected->tick;
    if (now_ > StandardMidiFile::MaximumDurationUs) return fail("FILE DURATION TOO LONG");
    auto& track = *selected;
    uint8_t status;
    if (!byte(reader, track, status)) return fail("TRUNCATED MIDI EVENT");
    if (status < 128) {
        if (!track.running) return fail("INVALID RUNNING STATUS");
        --track.position; status = track.running;
    }
    if (status < 0xF0) {
        track.running = status;
        uint8_t data[3]{status, 0, 0};
        const unsigned length = (status & 0xE0) == 0xC0 ? 2 : 3;
        for (unsigned i = 1; i < length; ++i)
            if (!byte(reader, track, data[i]) || data[i] >= 128) return fail("INVALID MIDI DATA");
        const MidiEvent event = parser.parseMessage(data[0], data[1], data[2], now_);
        output = {now_, event.type, event.channel, event.data1, event.data2, event.value14};
        result = Result::Event;
        if (event.type == MidiType::NoteOn && event.data2) channels_ |= 1U << event.channel;
    } else {
        track.running = 0;
        uint8_t meta = 0;
        if (status != 0xFF && status != 0xF0 && status != 0xF7) return fail("INVALID SYSTEM EVENT");
        if (status == 0xFF && !byte(reader, track, meta)) return fail("TRUNCATED META EVENT");
        uint32_t length;
        if (!variable(reader, track, length) || length > track.end - track.position) return fail("INVALID EVENT LENGTH");
        if (status == 0xFF && meta == 0x51) {
            uint8_t data[3]{};
            if (length != 3 || !reader.read(track.position, data, 3)) return fail("INVALID TEMPO");
            tempo_ = (uint32_t(data[0]) << 16) | (uint32_t(data[1]) << 8) | data[2];
            if (!tempo_) return fail("INVALID TEMPO");
        } else if (status == 0xFF && meta == 0x21) {
            uint8_t port = 0;
            if (length != 1 || !reader.read(track.position, &port, 1) || port != 0) return fail("MULTI-PORT MIDI UNSUPPORTED");
        } else if (status == 0xFF && meta == 0x2F) {
            if (length != 0 || track.position != track.end) return fail("INVALID END OF TRACK");
            track.done = true;
        } else if (status != 0xFF) ++skipped_sysex_;
        track.position += length;
    }
    if (!track.done) {
        uint32_t deltaTicks;
        if (!variable(reader, track, deltaTicks)) return fail("MISSING END OF TRACK");
        track.tick += deltaTicks;
    }
    return result;
}
bool StandardMidiFile::load(MidiFileReader& reader) {
    events.clear(); duration_us = 0; channels = 0; tracks = 0; skipped_sysex = 0;
    MidiFileDecoder decoder;
    if (!decoder.begin(reader)) { error = decoder.error(); return false; }
    MidiFileEvent event;
    for (;;) {
        const auto result = decoder.next(reader, event);
        if (result == MidiFileDecoder::Result::Error) { error = decoder.error(); events.clear(); return false; }
        if (result == MidiFileDecoder::Result::End) break;
        if (result == MidiFileDecoder::Result::Event && !events.push_back(event)) {
            error = "EVENT LIMIT IS 2048"; events.clear(); return false;
        }
    }
    duration_us = decoder.duration(); channels = decoder.channels(); tracks = decoder.tracks(); skipped_sysex = decoder.skippedSysex();
    error = nullptr;
    return true;
}
}
