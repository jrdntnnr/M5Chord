#include "engine/MidiLooper.h"
#include <algorithm>

namespace midibrain {

uint64_t MidiLooper::ticksAt(uint64_t nowUs) const {
    return nowUs <= origin_us_ ? 0 : (nowUs - origin_us_) * bpm_ * TicksPerQuarter / 60000000ULL;
}
uint64_t MidiLooper::micros(uint64_t ticks) const { return ticks * 60000000ULL / (bpm_ * TicksPerQuarter); }
LoopMode MidiLooper::mode() const { return mode_; }
uint32_t MidiLooper::length() const { return length_ticks_; }
uint16_t MidiLooper::bpm() const { return bpm_; }
uint32_t MidiLooper::dropped() const { return dropped_; }
uint16_t MidiLooper::sustainChannels() const {
    uint16_t channels = 0;
    for (const auto& entry : entries_) if (entry.type == MidiType::ControlChange && entry.data1 == 64) channels |= 1U << entry.channel;
    return channels;
}
const FixedList<LoopEntry, MidiLooper::Capacity>& MidiLooper::entries() const { return entries_; }

void MidiLooper::clear() {
    mode_ = LoopMode::Stopped;
    entries_.clear();
    pending_.fill({});
    emitted_.fill(false);
    length_ticks_ = 0;
    target_ticks_ = 0;
    layer_ = 0;
}

void MidiLooper::record(uint64_t nowUs, uint16_t bpm, uint8_t bars, uint16_t grid) {
    clear();
    origin_us_ = nowUs;
    bpm_ = std::clamp<uint16_t>(bpm, 30, 300);
    target_ticks_ = std::min<uint8_t>(bars, 16) * 4U * TicksPerQuarter;
    grid_ = grid;
    mode_ = LoopMode::Recording;
}

void MidiLooper::finishNotes(uint64_t nowUs) {
    for (auto& note : pending_) {
        if (!note.active) continue;
        auto& entry = entries_[note.entry];
        const uint64_t elapsed = nowUs > note.at ? (nowUs-note.at)*bpm_*TicksPerQuarter/60000000ULL : 1;
        entry.duration = static_cast<uint32_t>(std::clamp<uint64_t>(elapsed,1,MaximumTicks));
        note.active = false;
    }
    if (length_ticks_) {
        for (auto& entry : entries_) {
            entry.tick = std::min(entry.tick,length_ticks_-1);
            entry.duration = std::min(entry.duration,length_ticks_-entry.tick);
        }
    }
}

void MidiLooper::play(uint64_t nowUs) {
    if (mode_ == LoopMode::Recording) {
        length_ticks_ = target_ticks_ ? target_ticks_ : static_cast<uint32_t>(std::clamp<uint64_t>(ticksAt(nowUs),TicksPerQuarter,MaximumTicks));
        finishNotes(nowUs);
    } else if (mode_ == LoopMode::Overdub) finishNotes(nowUs);
    if (entries_.empty() || length_ticks_ == 0) { mode_ = LoopMode::Stopped; return; }
    origin_us_ = nowUs;
    cycle_ = 0;
    emitted_.fill(false);
    mode_ = LoopMode::Playing;
}

void MidiLooper::stop(uint64_t nowUs) {
    if (mode_ == LoopMode::Recording) {
        length_ticks_ = target_ticks_ ? target_ticks_ : static_cast<uint32_t>(std::clamp<uint64_t>(ticksAt(nowUs),TicksPerQuarter,MaximumTicks));
    }
    finishNotes(nowUs);
    mode_ = LoopMode::Stopped;
}

void MidiLooper::overdub(uint64_t nowUs) {
    if (mode_ == LoopMode::Overdub) { finishNotes(nowUs); mode_ = LoopMode::Playing; return; }
    if (entries_.empty() || !length_ticks_) return;
    if (mode_ != LoopMode::Playing) play(nowUs);
    if (layer_ == 65535) return;
    ++layer_;
    mode_ = LoopMode::Overdub;
}

void MidiLooper::undo(uint64_t nowUs) {
    stop(nowUs);
    if (!layer_) return;
    entries_.erase_if([this](const LoopEntry& entry) { return entry.layer == layer_; });
    --layer_;
}

void MidiLooper::capture(const MidiEvent& event, uint64_t nowUs) {
    if (mode_ != LoopMode::Recording && mode_ != LoopMode::Overdub) return;
    if (event.type == MidiType::NoteOff || (event.type == MidiType::NoteOn && !event.data2)) {
        for (auto& note : pending_) {
            if (!note.active || note.channel != event.channel || note.note != event.data1) continue;
            auto& entry = entries_[note.entry];
            const uint64_t elapsed = nowUs >= note.at ? (nowUs - note.at) * bpm_ * TicksPerQuarter / 60000000ULL : 1;
            entry.duration = static_cast<uint32_t>(std::max<uint64_t>(1, elapsed));
            const uint32_t limit = length_ticks_ ? length_ticks_ : target_ticks_;
            if (limit && entry.tick < limit) entry.duration = std::min(entry.duration, limit - entry.tick);
            note.active = false;
        }
        return;
    }
    if (event.type > MidiType::PitchBend) return;
    uint64_t tick = ticksAt(nowUs);
    if (mode_ == LoopMode::Recording && tick >= MaximumTicks) return;
    if (mode_ == LoopMode::Overdub) tick %= length_ticks_;
    if (target_ticks_ && mode_ == LoopMode::Recording && tick >= target_ticks_) return;
    if (grid_) tick = ((tick + grid_ / 2) / grid_) * grid_;
    const uint32_t limit = length_ticks_ ? length_ticks_ : target_ticks_;
    if (limit) tick = std::min<uint64_t>(tick, limit - 1);
    Pending* available = nullptr;
    if (event.type == MidiType::NoteOn) {
        for (auto& note : pending_) if (!note.active) { available = &note; break; }
        if (!available) { ++dropped_; return; }
    }
    if (!entries_.push_back({static_cast<uint32_t>(tick), 0, event.type, event.channel, event.data1, event.data2, event.value14, layer_})) {
        ++dropped_;
        return;
    }
    if (available) *available = {true, event.channel, event.data1, static_cast<uint16_t>(entries_.size() - 1), nowUs};
    if (mode_ == LoopMode::Overdub) emitted_[entries_.size() - 1] = true;
}

bool MidiLooper::tick(uint64_t nowUs, MidiScheduler& scheduler) {
    if (mode_ == LoopMode::Recording) {
        const uint32_t end = target_ticks_ ? target_ticks_ : MaximumTicks;
        if (ticksAt(nowUs) >= end) play(origin_us_ + micros(end));
        else return false;
    }
    if (mode_ != LoopMode::Playing && mode_ != LoopMode::Overdub) return false;
    const uint64_t current = ticksAt(nowUs);
    const uint64_t cycle = current / length_ticks_;
    const bool wrapped = cycle != cycle_;
    if (wrapped) {
        finishNotes(origin_us_ + micros(cycle * length_ticks_));
        emitted_.fill(false);
        scheduler.cancel(StreamId::Loop);
        cycle_ = cycle;
    }
    const uint32_t position = current % length_ticks_;
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        if (emitted_[i] || entry.tick > position) continue;
        emitted_[i] = true;
        const uint64_t due = origin_us_ + micros(cycle * length_ticks_ + entry.tick);
        const VoiceId owner{entry.channel, entry.data1, static_cast<uint32_t>(0x80000000U | (i + 1))};
        MidiEvent event{entry.type, entry.channel, entry.data1, entry.data2, entry.value14, due, 0};
        if (entry.type == MidiType::NoteOn) {
            if (!entry.duration) continue;
            const uint64_t off = origin_us_ + micros(cycle * length_ticks_ + std::min(length_ticks_, entry.tick + entry.duration));
            if (off <= nowUs) continue;
            if (scheduler.size() + 2 > MidiScheduler::Capacity) { ++dropped_; continue; }
            if (!scheduler.schedule(due, owner, StreamId::Loop, event)
                || !scheduler.schedule(off, owner, StreamId::Loop, MidiEvent::noteOff(entry.channel, entry.data1, 0, off))) {
                scheduler.cancel(owner);
                ++dropped_;
            }
        } else if (!scheduler.schedule(due, owner, StreamId::Loop, event)) ++dropped_;
    }
    return wrapped;
}

bool MidiLooper::replace(const FixedList<LoopEntry, Capacity>& entries, uint32_t ticks, uint16_t bpm) {
    if (!ticks || ticks > 96U * 4U * 1024U || bpm < 30 || bpm > 300) return false;
    for (const auto& e : entries) {
        if (e.type == MidiType::NoteOff || e.type > MidiType::PitchBend || e.channel > 15 || e.data1 > 127 || e.data2 > 127
            || e.value14 > 16383 || e.tick >= ticks || e.duration > ticks - e.tick
            || (e.type == MidiType::NoteOn && (!e.duration || !e.data2))) return false;
    }
    clear();
    entries_ = entries;
    length_ticks_ = ticks;
    bpm_ = bpm;
    for (const auto& e : entries_) layer_ = std::max(layer_, e.layer);
    return true;
}

}
