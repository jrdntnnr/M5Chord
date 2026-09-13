#include "TestSupport.h"
#include "app/App.h"
#include "hardware/InputKeys.h"
#include "midi/StandardMidiFile.h"
#include "midi/CachedMidiReader.h"
#include <algorithm>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>

using namespace midibrain;
namespace {
using Bytes = std::vector<uint8_t>;
void be32(Bytes& out, uint32_t value) { for (int i = 3; i >= 0; --i) out.push_back(value >> (8 * i)); }
Bytes smf(const std::vector<Bytes>& tracks, uint16_t division = 96, uint16_t format = 1) {
    Bytes out{'M','T','h','d',0,0,0,6,0,static_cast<uint8_t>(format),0,static_cast<uint8_t>(tracks.size()),static_cast<uint8_t>(division >> 8),static_cast<uint8_t>(division)};
    for (const auto& track : tracks) {
        out.insert(out.end(), {'M','T','r','k'}); be32(out, track.size()); out.insert(out.end(), track.begin(), track.end());
    }
    return out;
}
class MemoryReader final : public MidiFileReader {
public:
    explicit MemoryReader(const Bytes& bytes) : bytes_(bytes) {}
    uint32_t size() const override { return bytes_.size(); }
    bool read(uint32_t offset, uint8_t* data, std::size_t count) override {
        if (offset > bytes_.size() || count > bytes_.size() - offset) return false;
        std::memcpy(data, bytes_.data() + offset, count); return true;
    }
private:
    const Bytes& bytes_;
};
bool load(StandardMidiFile& file, const Bytes& data) { MemoryReader reader(data); return file.load(reader); }
std::size_t count(const test::FakeSink& sink, MidiType type) {
    return std::count_if(sink.events.begin(), sink.events.end(), [type](const MidiEvent& event) { return event.type == type; });
}
void action(void* context, const ActionEvent& event, uint64_t) { *static_cast<ActionEvent*>(context) = event; }
}

void testMidiFile() {
    StandardMidiFile file;
    const Bytes notes{0,0x92,60,100,48,62,80,48,60,0,0,62,0,0,0xFF,0x2F,0};
    EXPECT(load(file, smf({notes}, 96, 0)));
    EXPECT(file.events.size() == 4 && file.channels == 4 && file.duration_us == 500000);
    EXPECT(file.events[1].at_us == 250000 && file.events[2].type == MidiType::NoteOff);
    const Bytes tempo{0,0xFF,0x51,3,7,0xA1,0x20,48,0xFF,0x51,3,0x0F,0x42,0x40,48,0xFF,0x2F,0};
    EXPECT(load(file, smf({tempo, notes})));
    EXPECT(file.events[1].at_us == 250000 && file.events[2].at_us == 750000 && file.duration_us == 750000);
    EXPECT(!load(file, smf({notes}, 0xE728)));
    EXPECT(!load(file, smf({notes}, 0)));
    EXPECT(!load(file, smf({notes}, 96, 2)));
    EXPECT(!load(file, smf({notes, notes}, 96, 0)));
    EXPECT(!load(file, smf({{0,60,100,0,0xFF,0x2F,0}})));
    EXPECT(!load(file, smf({{0,0x90,60,128,0,0xFF,0x2F,0}})));
    EXPECT(!load(file, smf({{0x81,0x80,0x80,0x80,0,0x90,60,100}})));
    EXPECT(!load(file, smf({{0,0x90,60,100}})));
    EXPECT(!load(file, smf({{0,0xFF,0x51,3,0,0,0,0,0xFF,0x2F,0}})));
    EXPECT(!load(file, smf({{0,0xFF,0x2F,1,0}})));
    EXPECT(load(file, smf({{0,0xF0,2,1,0xF7,0,0x90,60,100,1,0x80,60,0,0,0xFF,0x2F,0}})));
    EXPECT(file.skipped_sysex == 1 && file.events.size() == 2);
    EXPECT(!load(file, smf({{0,0x90,60,100,0,0xFF,1,0,0,60,0,0,0xFF,0x2F,0}})));
    const auto valid = smf({notes});
    for (std::size_t length = 0; length < valid.size(); ++length) EXPECT(!load(file, Bytes(valid.begin(), valid.begin() + length)));
    Bytes crowded;
    for (unsigned i = 0; i <= StandardMidiFile::Capacity; ++i) crowded.insert(crowded.end(), {0,0x90,60,100});
    crowded.insert(crowded.end(), {0,0xFF,0x2F,0});
    EXPECT(!load(file, smf({crowded})) && file.events.empty());
    {
        const auto bytes = smf({crowded});
        MemoryReader raw(bytes); CachedMidiReader cached(raw); MidiFileDecoder decoder;
        EXPECT(decoder.begin(cached));
        MidiFileEvent event;
        unsigned decoded = 0;
        for (;;) {
            const auto result = decoder.next(cached, event);
            EXPECT(result != MidiFileDecoder::Result::Error);
            if (result == MidiFileDecoder::Result::End) break;
            if (result == MidiFileDecoder::Result::Event) ++decoded;
        }
        EXPECT(decoded == StandardMidiFile::Capacity + 1 && decoder.progress() == 100);
        uint8_t scratch[260]{};
        EXPECT(cached.read(125, scratch, sizeof(scratch)));
        EXPECT(!std::memcmp(scratch, bytes.data() + 125, sizeof(scratch)));
        EXPECT(!cached.read(bytes.size(), scratch, 1));
        cached.clear(); EXPECT(cached.read(0, scratch, 14));
    }
    {
        MidiFileStream stream;
        for (unsigned round = 0; round < 4; ++round) {
            for (unsigned i = 0; i < MidiFileStream::Capacity; ++i) EXPECT(stream.push({i, MidiType::NoteOn, 0, 60, 100, 0}));
            EXPECT(!stream.push({}));
            for (unsigned i = 0; i < MidiFileStream::Capacity; ++i) {
                MidiFileEvent event;
                EXPECT(stream.peek(event) && event.at_us == i); stream.pop();
            }
        }
        const auto token = stream.request();
        EXPECT(!stream.ready(token)); stream.reset(); stream.publish(token); EXPECT(stream.ready(token));
        stream.finish("READ FAILED"); EXPECT(stream.ended() && stream.error());
        stream.request(); EXPECT(!stream.ready(stream.requested()));
    }
    {
        MidiFileStream stream;
        std::atomic<bool> cancel{false};
        constexpr unsigned total = 100000;
        std::thread producer([&] {
            for (unsigned i = 0; i < total && !cancel.load(); ++i) {
                while (!stream.push({i, MidiType::NoteOn, 0, 60, 100, 0})) {
                    if (cancel.load()) return;
                    std::this_thread::yield();
                }
            }
            stream.finish();
        });
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        unsigned received = 0;
        bool ordered = true;
        while (received < total && std::chrono::steady_clock::now() < deadline) {
            MidiFileEvent event;
            if (stream.peek(event)) { ordered &= event.at_us == received; ++received; stream.pop(); }
            else std::this_thread::yield();
        }
        cancel.store(true); producer.join();
        EXPECT(ordered && received == total && stream.size() == 0);
    }

    test::FakeSink sink;
    App app(sink);
    std::vector<Bytes> tracks;
    for (uint8_t channel : {1,3,5,7,9}) tracks.push_back({0,static_cast<uint8_t>(0x90 | channel),60,100,96,static_cast<uint8_t>(0x80 | channel),60,0,0,0xFF,0x2F,0});
    EXPECT(load(app.midiFile().file(), smf(tracks)));
    app.apply({SemanticAction::MidiFilePlay,1,true}, 1000);
    sink.events.clear();
    app.tick(1000);
    EXPECT(count(sink, MidiType::NoteOn) == 4 && app.activeNotes().size() == 4);
    for (unsigned i = 0; i < 4; ++i) EXPECT(sink.events[i].channel == i);
    EXPECT(!app.idle() && app.midiFile().playing());
    app.tick(500999);
    EXPECT(count(sink, MidiType::NoteOff) == 0);
    app.tick(501000);
    EXPECT(count(sink, MidiType::NoteOff) == 4 && app.activeNotes().empty() && app.scheduler().empty());
    EXPECT(!app.midiFile().playing() && app.idle());
    EXPECT(app.looper().entries().empty());
    app.apply({SemanticAction::LaneCountSet,1,true}, 502000);
    app.apply({SemanticAction::MidiFilePlay,1,true}, 503000);
    sink.events.clear(); app.tick(503000);
    EXPECT(count(sink, MidiType::NoteOn) == 1);
    app.apply({SemanticAction::LaneCountSet,16,true}, 504000);
    EXPECT(!app.midiFile().playing() && app.activeNotes().empty());
    app.apply({SemanticAction::MidiFilePlay,1,true}, 505000);
    sink.events.clear(); app.tick(505000);
    EXPECT(count(sink, MidiType::NoteOn) == 5);
    app.panic(506000);
    EXPECT(app.activeNotes().empty() && app.scheduler().empty() && !app.midiFile().playing());
    app.tick(2000000);
    EXPECT(app.activeNotes().empty());

    EXPECT(load(app.midiFile().file(), smf({{0,0x90,60,100,0,60,90,1,60,0,1,60,0,0,0xFF,0x2F,0}})));
    app.apply({SemanticAction::MidiFilePlay,1,true}, 3000000);
    sink.events.clear(); app.tick(3000000);
    EXPECT(app.activeNotes().size() == 2 && count(sink, MidiType::NoteOn) == 1);
    app.tick(3006000);
    EXPECT(app.activeNotes().size() == 1 && count(sink, MidiType::NoteOff) == 0);
    app.tick(3011000);
    EXPECT(app.activeNotes().empty() && count(sink, MidiType::NoteOff) == 1);

    EXPECT(load(app.midiFile().file(), smf({{0,0xB0,64,127,0,0x90,60,100,96,0xFF,0x2F,0}})));
    app.apply({SemanticAction::MidiFilePlay,1,true}, 4000000);
    sink.events.clear(); app.tick(4000000); app.tick(4500000);
    EXPECT(count(sink, MidiType::NoteOff) == 1 && app.activeNotes().empty());
    EXPECT(std::any_of(sink.events.begin(), sink.events.end(), [](const MidiEvent& e) { return e.type == MidiType::ControlChange && e.data1 == 64 && e.data2 == 0; }));

    EXPECT(!load(file, smf({{0,0xFF,0x21,1,1,0,0x90,60,100,1,0x80,60,0,0,0xFF,0x2F,0}})));
    EXPECT(load(app.midiFile().file(), smf({notes})));
    app.state().mode = EngineMode::Bypass;
    app.apply({SemanticAction::MidiFilePlay,1,true}, 5000000);
    app.tick(5000000);
    app.receive(MidiEvent::noteOn(0,60,100,5000001)); app.tick(5000001);
    sink.events.clear(); app.stopMidiFile(5000002);
    EXPECT(app.activeNotes().size() == 1 && count(sink, MidiType::NoteOff) == 0);
    app.receive(MidiEvent::noteOff(0,60,0,5000003)); app.tick(5000003);
    EXPECT(app.activeNotes().empty() && count(sink, MidiType::NoteOff) == 1);
    app.apply({SemanticAction::MidiFilePlay,1,true}, 6000000); app.tick(6000000);
    app.onUsbDisconnected(6000001);
    EXPECT(!app.midiFile().playing() && app.activeNotes().empty() && app.scheduler().empty());

    MidiFilePlayer player;
    EXPECT(load(player.file(), smf({notes})));
    EXPECT(player.play(0,4));
    MidiScheduler full;
    for (unsigned i = 0; i < MidiScheduler::Capacity; ++i) EXPECT(full.schedule(10000000, {}, StreamId::Loop, MidiEvent::cc(0,1,1)));
    EXPECT(!player.tick(0, full));
    player.stop();
    EXPECT(std::strstr(player.status(), "FULL"));
    Bytes manyNotes;
    for (unsigned i = 0; i < 257; ++i) manyNotes.insert(manyNotes.end(), {0,0x90,60,100});
    manyNotes.insert(manyNotes.end(), {96,0xFF,0x2F,0});
    EXPECT(load(app.midiFile().file(), smf({manyNotes})));
    app.apply({SemanticAction::MidiFilePlay,1,true}, 7000000);
    for (unsigned i = 0; i < 5; ++i) app.tick(7000000+i);
    EXPECT(!app.midiFile().playing() && app.activeNotes().empty() && app.scheduler().empty());

    const Bytes expression{0,0xC3,10,0,0xB3,7,99,0,0xE3,0,64,0,0xD3,80,0,0x93,60,100,0,0xA3,60,70,96,0x83,60,0,0,0xFF,0x2F,0};
    EXPECT(load(app.midiFile().file(), smf({expression})));
    app.apply({SemanticAction::MidiFilePlay,1,true}, 8000000);
    sink.events.clear(); app.tick(8000000);
    EXPECT(count(sink, MidiType::ProgramChange) == 1 && count(sink, MidiType::PitchBend) == 1);
    EXPECT(count(sink, MidiType::ChannelPressure) == 1 && count(sink, MidiType::PolyAftertouch) == 1);
    for (const auto& event : sink.events) EXPECT(event.channel == 0);
    app.panic(8000001);

    MidiFileCatalog catalog;
    {
        test::FakeSink isolatedSink;
        App isolated(isolatedSink);
        isolated.state().mode = EngineMode::Bypass;
        EXPECT(load(isolated.midiFile().file(), smf({{0,0x90,60,100,96,0x80,60,0,0,0xFF,0x2F,0}})));
        isolated.apply({SemanticAction::MidiFilePlay,1,true}, 100);
        isolated.tick(100);
        isolated.receive(MidiEvent::noteOn(0,60,100,101)); isolated.tick(101);
        EXPECT(isolated.activeNotes().size() == 2);
        isolated.receive(MidiEvent::noteOff(0,60,0,102)); isolated.tick(102);
        EXPECT(isolated.activeNotes().size() == 1 && isolated.midiFile().playing());
        isolated.tick(500100);
        EXPECT(isolated.activeNotes().empty());
    }
    EXPECT(!catalog.add("../escape.mid") && !catalog.add(".hidden.mid") && !catalog.add("song.txt"));
    EXPECT(catalog.add("Zulu.MID") && catalog.add("Alpha.midi"));
    EXPECT(!catalog.add("Zulu.MID") && !std::strcmp(catalog.names[0].data(), "Alpha.midi"));
    ControllerMapper mapper; ProfileStore profiles; ActionEvent captured;
    InputKeys keys(app, mapper, profiles, action, &captured);
    keys.fileCatalog() = catalog;
    keys.openFiles();
    EXPECT(keys.menuOpen() && keys.menuCount() == 2 && keys.menuIndex() == 0);
    keys.handleAction({SemanticAction::MenuUp,1,true}, 0);
    EXPECT(keys.menuIndex() == 1);
    keys.handleAction({SemanticAction::MenuConfirm,1,true}, 0);
    EXPECT(!keys.browsingFiles() && captured.action == SemanticAction::MidiFileLoad && captured.value == 1);
    keys.openFiles(); keys.handleAction({SemanticAction::HelpToggle,1,true}, 0);
    EXPECT(!keys.browsingFiles() && keys.menuOpen());
    keys.fileCatalog().names.clear(); keys.openFiles();
    EXPECT(keys.menuCount() == 1);
    captured = {};
    keys.handleAction({SemanticAction::MenuConfirm,1,true}, 0);
    EXPECT(captured.action == SemanticAction::None);
    keys.handleAction({SemanticAction::HelpToggle,1,true}, 0);
    EXPECT(keys.playerOpen());
    keys.selectMidiPlayback();
    keys.setFileLoading(true, 42);
    keys.handleAction({SemanticAction::MenuConfirm,1,true}, 0);
    EXPECT(captured.action == SemanticAction::None && keys.fileProgress() == 42);
    keys.setFileLoading(false, 100);
    keys.handleAction({SemanticAction::MenuConfirm,1,true}, 0);
    EXPECT(captured.action == SemanticAction::MidiFilePlay);
    keys.handleAction({SemanticAction::MenuDecrease,1,true}, 0);
    keys.handleAction({SemanticAction::MenuConfirm,1,true}, 0);
    EXPECT(captured.action == SemanticAction::MidiFileBrowse);
    keys.handleAction({SemanticAction::HelpToggle,1,true}, 0);
    EXPECT(!keys.playerOpen() && keys.menuOpen());
    {
        test::FakeSink output; App streaming(output); MidiFileStream stream;
        streaming.midiFile().file().error = nullptr;
        streaming.midiFile().file().channels = 1;
        streaming.midiFile().file().duration_us = 1000000;
        streaming.midiFile().attach(&stream);
        streaming.apply({SemanticAction::MidiFilePlay,1,true}, 0);
        EXPECT(streaming.midiFile().buffering());
        streaming.tick(100);
        EXPECT(streaming.activeNotes().empty());
        stream.reset();
        EXPECT(stream.push({0, MidiType::NoteOn, 0, 60, 100, 0}));
        EXPECT(stream.push({1000000, MidiType::NoteOff, 0, 60, 0, 0}));
        stream.publish(stream.requested());
        streaming.tick(200);
        EXPECT(streaming.activeNotes().size() == 1);
        stream.finish("SD READ FAILED");
        streaming.tick(300);
        EXPECT(!streaming.midiFile().playing() && streaming.activeNotes().empty() && streaming.scheduler().empty());
        EXPECT(!std::strcmp(streaming.midiFile().status(), "SD READ FAILED"));
        streaming.apply({SemanticAction::MidiFilePlay,1,true}, 400);
        stream.reset(); stream.publish(stream.requested());
        streaming.tick(500);
        EXPECT(!streaming.midiFile().playing() && std::strstr(streaming.midiFile().status(), "SD TOO SLOW"));
        streaming.apply({SemanticAction::MidiFilePlay,1,true}, 600);
        streaming.panic(601); stream.reset(); stream.publish(stream.requested());
        streaming.tick(602);
        EXPECT(!streaming.midiFile().playing() && streaming.activeNotes().empty());
    }

    uint32_t random = 17;
    for (unsigned i = 0; i < 2000; ++i) {
        Bytes mutated = valid;
        random = random * 1664525U + 1013904223U;
        mutated[random % mutated.size()] ^= static_cast<uint8_t>(random >> 24);
        load(file, mutated);
    }
}
