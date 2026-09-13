#include "storage/ProfileCodec.h"
#include <cstring>
#include <cstdio>
#include <memory>
#include <new>

namespace midibrain {
namespace {
const char* types[]{"note","cc","program_change","channel_pressure","pitch_bend"};
constexpr MidiType midiTypes[]{MidiType::NoteOn, MidiType::ControlChange, MidiType::ProgramChange, MidiType::ChannelPressure, MidiType::PitchBend};
const char* triggers[]{"press","release","press_release","toggle","value","relative"};
const char* relatives[]{"twos_complement","binary_offset","signed_bit"};
int lookup(const char* name, const char* const* list, std::size_t size) {
    if (!name) return -1;
    for (std::size_t i = 0; i < size; ++i) if (!std::strcmp(name,list[i])) return static_cast<int>(i);
    return -1;
}
bool integer(JsonVariantConst v, int minimum, int maximum, int& out) {
    if (!v.is<int>()) return false;
    out = v.as<int>();
    return out >= minimum && out <= maximum;
}
bool textField(JsonVariantConst v, char* target, const char* fallback) {
    if (!v.isNull() && !v.is<const char*>()) return false;
    const char* value = v.isNull() ? fallback : v.as<const char*>();
    if (std::strlen(value) > 64) return false;
    std::snprintf(target, 65, "%s", value);
    return true;
}
}
bool ProfileCodec::decode(const JsonDocument& d, ControllerProfile& out, const char*& error) {
    error = "Invalid controller profile";
    if (d["schema"] != 1 || !d["mappings"].is<JsonArrayConst>()) return false;
    std::unique_ptr<ControllerProfile> scratch(new (std::nothrow) ControllerProfile);
    if (!scratch) { error = "Profile memory unavailable"; return false; }
    auto& parsed = *scratch;
    if (!textField(d["name"], parsed.name, "Generic")
        || !textField(d["match"]["manufacturer_contains"], parsed.manufacturer, "")
        || !textField(d["match"]["product_contains"], parsed.product, "")) return false;
    int number;
    if (!d["match"]["vid"].isNull()) {
        if (!integer(d["match"]["vid"],0,65535,number)) return false;
        parsed.vid = number;
    }
    if (!d["match"]["pid"].isNull()) {
        if (!integer(d["match"]["pid"],0,65535,number)) return false;
        parsed.pid = number;
    }
    const auto input = d["input"];
    if (!input["channel"].isNull() && input["channel"] != "omni") {
        if (!integer(input["channel"],1,16,number)) return false;
        parsed.input_channel = number - 1;
    }
    if (!input["root_note_low"].isNull()) {
        if (!integer(input["root_note_low"],0,127,number)) return false;
        parsed.low = number;
    }
    if (!input["root_note_high"].isNull()) {
        if (!integer(input["root_note_high"],parsed.low,127,number)) return false;
        parsed.high = number;
    }
    const JsonArrayConst mappings = d["mappings"];
    if (mappings.size() > ControllerMapper::MaxMappings) return false;
    for (JsonObjectConst item : mappings) {
        if (item.isNull()) return false;
        const int type = lookup(item["source"]["type"],types,std::size(types));
        const int trigger = lookup(item["trigger"] | "press",triggers,std::size(triggers));
        const int relative = lookup(item["relative_mode"] | "twos_complement",relatives,std::size(relatives));
        const char* name = item["action"] | "";
        if (std::strlen(name) > 64 || type < 0 || trigger < 0 || relative < 0) return false;
        const SemanticAction action = ControllerMapper::actionFromName(name);
        if (action == SemanticAction::None) { error = "Unknown action"; return false; }
        ControllerMapping m;
        m.action = action;
        m.source.type = midiTypes[type];
        m.trigger = static_cast<MappingTrigger>(trigger);
        m.relative_mode = static_cast<RelativeMode>(relative);
        if (!item["consume"].isNull() && !item["consume"].is<bool>()) return false;
        m.consume = item["consume"] | false;
        const auto source = item["source"];
        if (source["channel"] != "omni") {
            if (!integer(source["channel"],1,16,number)) return false;
            m.source.channel = number - 1;
        }
        if (!source["number"].isNull()) {
            if (!integer(source["number"],-1,127,number)) return false;
            m.source.number = number;
        }
        if (!source["cable"].isNull()) {
            if (!integer(source["cable"],-1,15,number)) return false;
            m.source.cable = number;
        }
        if (!source["value_min"].isNull()) {
            if (!integer(source["value_min"],0,127,number)) return false;
            m.source.minimum = number;
        }
        if (!source["value_max"].isNull()) {
            if (!integer(source["value_max"],m.source.minimum,127,number)) return false;
            m.source.maximum = number;
        }
        if ((action >= SemanticAction::ChordDim && action <= SemanticAction::Extension9)
            && (m.source.type == MidiType::NoteOn || m.source.type == MidiType::ControlChange)
            && m.trigger == MappingTrigger::Press) m.trigger = MappingTrigger::PressRelease;
        parsed.mapper.add(m);
    }
    out = parsed;
    error = "";
    return true;
}
void ProfileCodec::encode(const ControllerProfile& p, JsonDocument& d) {
    d.clear();
    d["schema"] = 1;
    d["name"] = p.name;
    if (p.vid >= 0) d["match"]["vid"] = p.vid;
    if (p.pid >= 0) d["match"]["pid"] = p.pid;
    if (p.manufacturer[0]) d["match"]["manufacturer_contains"] = p.manufacturer;
    if (p.product[0]) d["match"]["product_contains"] = p.product;
    if (p.input_channel < 0) d["input"]["channel"] = "omni";
    else d["input"]["channel"] = p.input_channel + 1;
    d["input"]["root_note_low"] = p.low;
    d["input"]["root_note_high"] = p.high;
    auto mappings = d["mappings"].to<JsonArray>();
    for (std::size_t i = 0; i < p.mapper.size(); ++i) {
        const auto& m = p.mapper.mapping(i);
        auto item = mappings.add<JsonObject>();
        for (std::size_t t = 0; t < std::size(types); ++t) if (m.source.type == midiTypes[t]) item["source"]["type"] = types[t];
        if (m.source.channel < 0) item["source"]["channel"] = "omni";
        else item["source"]["channel"] = m.source.channel + 1;
        item["source"]["number"] = m.source.number;
        item["source"]["cable"] = m.source.cable;
        item["source"]["value_min"] = m.source.minimum;
        item["source"]["value_max"] = m.source.maximum;
        item["trigger"] = triggers[static_cast<uint8_t>(m.trigger)];
        item["relative_mode"] = relatives[static_cast<uint8_t>(m.relative_mode)];
        item["action"] = ControllerMapper::actionName(m.action);
        item["consume"] = m.consume;
    }
}
int ProfileCodec::match(const ControllerProfile& p, uint16_t vid, uint16_t pid, const char* manufacturer, const char* product) {
    if (p.vid >= 0 && p.pid >= 0) return p.vid == vid && p.pid == pid ? 30 : 0;
    const bool productMatch = p.product[0] && product && std::strstr(product,p.product);
    const bool manufacturerMatch = p.manufacturer[0] && manufacturer && std::strstr(manufacturer,p.manufacturer);
    if (productMatch && manufacturerMatch) return 20;
    if (productMatch && !p.manufacturer[0]) return 10;
    return 0;
}
}
