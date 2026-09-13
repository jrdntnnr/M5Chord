#include "app/App.h"
#include "midi/CachedMidiReader.h"
#include <fstream>
#include <iostream>
#include <array>

using namespace midibrain;
namespace {
class Reader final : public MidiFileReader {
public:
    explicit Reader(const char* path) : input_(path, std::ios::binary | std::ios::ate) {
        if (input_) bytes_ = static_cast<uint32_t>(input_.tellg());
    }
    uint32_t size() const override { return bytes_; }
    bool read(uint32_t offset, uint8_t* data, std::size_t count) override {
        ++reads;
        if (offset > bytes_ || count > bytes_ - offset) return false;
        input_.clear(); input_.seekg(offset);
        return static_cast<bool>(input_.read(reinterpret_cast<char*>(data), count));
    }
    unsigned reads{0};
private:
    std::ifstream input_;
    uint32_t bytes_{0};
};
class Sink final : public MidiSink {
public:
    bool send(const MidiEvent& event) override {
        if (event.type == MidiType::NoteOn) ++notes[event.channel];
        if (event.type == MidiType::NoteOff) ++offs[event.channel];
        return true;
    }
    std::array<unsigned, 16> notes{}, offs{};
};
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: midi_file_check file.mid\n"; return 2; }
    Reader source(argv[1]); CachedMidiReader reader(source); MidiFileDecoder decoder;
    if (!decoder.begin(reader)) { std::cerr << decoder.error() << '\n'; return 1; }
    MidiFileEvent event;
    unsigned events = 0;
    for (;;) {
        const auto result = decoder.next(reader, event);
        if (result == MidiFileDecoder::Result::Error) { std::cerr << decoder.error() << '\n'; return 1; }
        if (result == MidiFileDecoder::Result::End) break;
        if (result == MidiFileDecoder::Result::Event) ++events;
    }
    std::cout << "Validated " << source.size() << " bytes, " << events << " events, " << decoder.tracks() << " tracks, " << decoder.duration() << " us; " << source.reads << " cached source reads\n";
    Sink sink; App app(sink); MidiFileStream stream;
    auto& file = app.midiFile().file();
    file.duration_us = decoder.duration(); file.channels = decoder.channels(); file.error = nullptr;
    app.midiFile().attach(&stream);
    for (unsigned run = 0; run < 2; ++run) {
        const uint64_t origin = run * (file.duration_us + 1);
        app.apply({SemanticAction::MidiFilePlay, 1, true}, origin);
        stream.reset(); reader.clear();
        if (!decoder.begin(reader)) return 1;
        uint64_t now = origin;
        unsigned iterations = 0;
        do {
            while (!stream.ended() && stream.size() < MidiFileStream::Capacity) {
                const auto result = decoder.next(reader, event);
                if (result == MidiFileDecoder::Result::Error) { std::cerr << decoder.error() << '\n'; return 1; }
                if (result == MidiFileDecoder::Result::End) stream.finish();
                if (result == MidiFileDecoder::Result::Event) stream.push(event);
            }
            stream.publish(stream.requested());
            app.tick(now);
            if (stream.peek(event)) now = std::max(now, origin + event.at_us);
            else now = origin + file.duration_us;
            if (++iterations > 2000000) { std::cerr << "Playback did not finish\n"; return 1; }
        } while (app.midiFile().playing());
        if (!app.activeNotes().empty() || !app.scheduler().empty() || app.state().stats.midi_events_dropped) {
            std::cerr << "Playback failed: " << app.midiFile().status() << '\n'; return 1;
        }
        for (unsigned channel = 4; channel < 16; ++channel) if (sink.notes[channel]) return 1;
    }
    for (unsigned channel = 0; channel < 4; ++channel) {
        std::cout << "Output " << channel + 1 << ": " << sink.notes[channel] / 2 << " attacks, " << sink.offs[channel] / 2 << " releases per play\n";
        if (sink.notes[channel] != sink.offs[channel]) return 1;
    }
    std::cout << "Two complete buffered plays; four-channel limit; no dropped events or remaining note owners\n";
}
