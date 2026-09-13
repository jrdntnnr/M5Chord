#include "storage/LoopCodec.h"
#include <memory>
#include <new>

namespace midibrain {
namespace {
uint32_t checksum(const FixedList<LoopEntry,MidiLooper::Capacity>& entries, uint32_t ticks, uint16_t bpm) {
    uint32_t hash = 2166136261U;
    const auto add = [&hash](uint32_t value) {
        for (unsigned shift=0;shift<32;shift+=8) { hash ^= (value>>shift)&255U; hash *= 16777619U; }
    };
    add(ticks);
    add(bpm);
    for (const auto& e : entries) {
        add(e.tick); add(e.duration); add(static_cast<uint32_t>(e.type)); add(e.channel);
        add(e.data1); add(e.data2); add(e.value14); add(e.layer);
    }
    return hash;
}
}
void LoopCodec::encode(const MidiLooper& loop, JsonDocument& d) {
    d.clear();
    d["schema"] = 1;
    d["ticks"] = loop.length();
    d["bpm"] = loop.bpm();
    d["checksum"] = checksum(loop.entries(),loop.length(),loop.bpm());
    auto entries = d["events"].to<JsonArray>();
    for (const auto& e : loop.entries()) {
        auto a = entries.add<JsonArray>();
        a.add(e.tick); a.add(e.duration); a.add(static_cast<int>(e.type)); a.add(e.channel);
        a.add(e.data1); a.add(e.data2); a.add(e.value14); a.add(e.layer);
    }
}
bool LoopCodec::decode(const JsonDocument& d, MidiLooper& loop) {
    if (d["schema"] != 1 || !d["events"].is<JsonArrayConst>()) return false;
    if (!d["ticks"].is<uint32_t>() || !d["bpm"].is<uint16_t>() || !d["checksum"].is<uint32_t>()) return false;
    const auto array = d["events"].as<JsonArrayConst>();
    if (array.size() > MidiLooper::Capacity) return false;
    std::unique_ptr<FixedList<LoopEntry,MidiLooper::Capacity>> entries(new (std::nothrow) FixedList<LoopEntry,MidiLooper::Capacity>);
    if (!entries) return false;
    for (JsonArrayConst a : array) {
        if (a.size() != 8) return false;
        constexpr uint32_t maxima[]{393216,393216,6,15,127,127,16383,65535};
        for (unsigned i=0;i<8;++i) if (!a[i].is<uint32_t>() || a[i].as<uint32_t>() > maxima[i]) return false;
        entries->push_back({a[0],a[1],static_cast<MidiType>(a[2].as<int>()),a[3],a[4],a[5],a[6],a[7]});
    }
    if (checksum(*entries,d["ticks"],d["bpm"]) != d["checksum"].as<uint32_t>()) return false;
    return loop.replace(*entries,d["ticks"],d["bpm"]);
}
}
