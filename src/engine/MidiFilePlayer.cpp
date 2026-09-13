#include "engine/MidiFilePlayer.h"
#include <algorithm>
#include <cstdio>
#include <limits>

namespace midibrain {
void MidiFilePlayer::setName(const char* name) { failed_ = false; elapsed_us_ = 0; std::snprintf(name_, sizeof(name_), "%s", name ? name : ""); }
bool MidiFilePlayer::play(uint64_t nowUs, uint8_t channels) {
    failed_ = false;
    stop();
    if (!file_.valid()) { status_ = file_.error; return false; }
    if (nowUs > std::numeric_limits<uint64_t>::max() - file_.duration_us) { status_ = "TIME OUT OF RANGE"; return false; }
    map_.fill(-1); used_channels_ = 0;
    for (unsigned i = 0; i < 16; ++i)
        if ((file_.channels & (1U << i)) && used_channels_ < std::clamp<unsigned>(channels, 1, 16)) map_[i] = used_channels_++;
    next_ = 0; origin_us_ = nowUs; elapsed_us_ = 0; playing_ = true; status_ = "PLAYING";
    waiting_ = stream_ != nullptr;
    if (stream_) { token_ = stream_->request(); status_ = "BUFFERING"; }
    return true;
}
void MidiFilePlayer::stop() { playing_ = false; waiting_ = false; held_.fill({}); if (!failed_) status_ = file_.valid() ? "READY" : file_.error; }
bool MidiFilePlayer::finished(uint64_t nowUs) const {
    return playing_ && !waiting_ && (stream_ ? stream_->ended() && stream_->size() == 0 : next_ == file_.events.size()) && nowUs >= origin_us_ && nowUs - origin_us_ >= file_.duration_us;
}
bool MidiFilePlayer::tick(uint64_t nowUs, MidiScheduler& scheduler) {
    if (!playing_) return true;
    if (waiting_) {
        if (!stream_->ready(token_)) return true;
        waiting_ = false; origin_us_ = nowUs; status_ = "PLAYING";
    }
    elapsed_us_ = std::min(nowUs >= origin_us_ ? nowUs - origin_us_ : 0, file_.duration_us);
    if (stream_ && stream_->error()) { failed_ = true; status_ = stream_->error(); return false; }
    unsigned budget = 64;
    while (budget--) {
        MidiFileEvent source;
        if (stream_) {
            if (!stream_->peek(source)) {
                if (!stream_->ended()) { failed_ = true; status_ = "SD TOO SLOW: STOPPED"; return false; }
                break;
            }
        } else {
            if (next_ == file_.events.size()) break;
            source = file_.events[next_];
        }
        const uint64_t due = origin_us_ + source.at_us;
        if (due > nowUs) break;
        if (stream_) stream_->pop();
        const auto generation = 0x40000000U | static_cast<uint32_t>(++next_);
        const int8_t channel = map_[source.channel];
        if (channel < 0 || (source.type == MidiType::ControlChange && source.data1 >= 120)) continue;
        VoiceId owner{source.channel, source.data1, generation};
        if (source.type == MidiType::NoteOn) {
            Held* slot = nullptr;
            for (auto& held : held_) if (!held.generation) { slot = &held; break; }
            if (!slot) { failed_ = true; status_ = "FILE POLYPHONY LIMIT"; return false; }
            *slot = {generation, source.channel, source.data1};
        } else if (source.type == MidiType::NoteOff) {
            Held* oldest = nullptr;
            for (auto& held : held_)
                if (held.generation && held.channel == source.channel && held.note == source.data1 &&
                    (!oldest || held.generation < oldest->generation)) oldest = &held;
            if (!oldest) continue;
            owner.generation = oldest->generation;
            oldest->generation = 0;
        }
        MidiEvent event{source.type, static_cast<uint8_t>(channel), source.data1, source.data2, source.value14, due, 0};
        if (!scheduler.schedule(due, owner, StreamId::File, event)) { failed_ = true; status_ = "FILE SCHEDULER FULL"; return false; }
    }
    return true;
}
}
