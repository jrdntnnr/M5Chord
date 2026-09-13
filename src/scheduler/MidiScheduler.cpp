#include "scheduler/MidiScheduler.h"

#include <algorithm>
#include <limits>

namespace midibrain {

namespace {

bool critical(const MidiEvent& event) {
    return event.type == MidiType::NoteOff
        || event.type == MidiType::Stop
        || (event.type == MidiType::ControlChange && (event.data1 == 64 || event.data1 == 120 || event.data1 == 121 || event.data1 == 123));
}

bool replaceable(const MidiEvent& event) {
    return event.type == MidiType::NoteOn
        || event.type == MidiType::ControlChange
        || event.type == MidiType::PolyAftertouch
        || event.type == MidiType::ChannelPressure
        || event.type == MidiType::PitchBend;
}

}

bool MidiScheduler::earlier(const ScheduledMidiEvent& left, const ScheduledMidiEvent& right) const {
    return left.due_us < right.due_us || (left.due_us == right.due_us && left.sequence < right.sequence);
}

bool MidiScheduler::schedule(uint64_t dueUs, const VoiceId& owner, StreamId stream, const MidiEvent& event) {
    if (size_ == Capacity) {
        ++stats_.dropped;
        if (!critical(event)) return false;
        for (std::size_t i = 0; i < size_; ++i) {
            if (replaceable(heap_[i].event) && !critical(heap_[i].event)) {
                heap_[i] = {dueUs, next_sequence_++, owner, stream, event};
                rebuild();
                return true;
            }
        }
        return false;
    }
    heap_[size_] = {dueUs, next_sequence_++, owner, stream, event};
    siftUp(size_++);
    stats_.high_water = std::max(stats_.high_water, size_);
    return true;
}

bool MidiScheduler::popDue(uint64_t nowUs, ScheduledMidiEvent& event) {
    if (size_ == 0 || heap_[0].due_us > nowUs) {
        return false;
    }
    event = heap_[0];
    heap_[0] = heap_[--size_];
    if (size_ > 0) {
        siftDown(0);
    }
    const uint64_t late = nowUs - event.due_us;
    stats_.last_late_us = static_cast<uint32_t>(std::min<uint64_t>(late, std::numeric_limits<uint32_t>::max()));
    if (late > 0) {
        ++stats_.late_count;
        stats_.total_late_us += late;
        stats_.max_late_us = std::max(stats_.max_late_us, stats_.last_late_us);
    }
    return true;
}

std::size_t MidiScheduler::cancel(const VoiceId& owner) {
    const std::size_t before = size_;
    std::size_t write = 0;
    for (std::size_t read = 0; read < size_; ++read) {
        if (heap_[read].owner != owner) {
            heap_[write++] = heap_[read];
        }
    }
    size_ = write;
    rebuild();
    return before - size_;
}

std::size_t MidiScheduler::cancel(StreamId stream) {
    const std::size_t before = size_;
    std::size_t write = 0;
    for (std::size_t read = 0; read < size_; ++read) {
        if (heap_[read].stream != stream) {
            heap_[write++] = heap_[read];
        }
    }
    size_ = write;
    rebuild();
    return before - size_;
}

void MidiScheduler::clear() { size_ = 0; }
std::size_t MidiScheduler::size() const { return size_; }
bool MidiScheduler::empty() const { return size_ == 0; }
const SchedulerStats& MidiScheduler::stats() const { return stats_; }

void MidiScheduler::siftUp(std::size_t index) {
    while (index > 0) {
        const std::size_t parent = (index - 1) / 2;
        if (!earlier(heap_[index], heap_[parent])) {
            break;
        }
        std::swap(heap_[index], heap_[parent]);
        index = parent;
    }
}

void MidiScheduler::siftDown(std::size_t index) {
    for (;;) {
        const std::size_t left = index * 2 + 1;
        const std::size_t right = left + 1;
        std::size_t smallest = index;
        if (left < size_ && earlier(heap_[left], heap_[smallest])) {
            smallest = left;
        }
        if (right < size_ && earlier(heap_[right], heap_[smallest])) {
            smallest = right;
        }
        if (smallest == index) {
            break;
        }
        std::swap(heap_[index], heap_[smallest]);
        index = smallest;
    }
}

void MidiScheduler::rebuild() {
    if (size_ < 2) {
        return;
    }
    for (std::size_t i = size_ / 2; i-- > 0;) {
        siftDown(i);
    }
}

}
