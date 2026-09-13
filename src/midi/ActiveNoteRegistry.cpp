#include "midi/ActiveNoteRegistry.h"

namespace midibrain {

bool ActiveNoteRegistry::pitchActive(uint8_t channel, uint8_t note) const {
    for (const auto& active : notes_) {
        if (active.active && active.output_channel == channel && active.note == note) {
            return true;
        }
    }
    return false;
}

bool ActiveNoteRegistry::activate(const VoiceId& owner, StreamId stream, uint8_t channel, uint8_t note, bool& shouldSend) {
    for (const auto& active : notes_) {
        if (active.active && active.owner == owner && active.stream == stream && active.output_channel == channel && active.note == note) {
            shouldSend = false;
            return true;
        }
    }
    shouldSend = !pitchActive(channel, note);
    for (auto& slot : notes_) {
        if (!slot.active) {
            slot = {owner, stream, channel, note, true};
            return true;
        }
    }
    shouldSend = false;
    return false;
}

bool ActiveNoteRegistry::deactivate(const VoiceId& owner, StreamId stream, uint8_t channel, uint8_t note, bool& shouldSend) {
    for (auto& active : notes_) {
        if (active.active && active.owner == owner && active.stream == stream && active.output_channel == channel && active.note == note) {
            active.active = false;
            shouldSend = !pitchActive(channel, note);
            return true;
        }
    }
    shouldSend = false;
    return false;
}

bool ActiveNoteRegistry::owns(const VoiceId& owner, StreamId stream, uint8_t channel, uint8_t note) const {
    for (const auto& active : notes_) {
        if (active.active && active.owner == owner && active.stream == stream && active.output_channel == channel && active.note == note) {
            return true;
        }
    }
    return false;
}

FixedList<OutputNote, ActiveNoteRegistry::Capacity> ActiveNoteRegistry::collectReleased() {
    FixedList<OutputNote, Capacity> result;
    for (const auto& candidate : pending_releases_) {
        if (pitchActive(candidate.channel, candidate.note)) {
            continue;
        }
        bool duplicate = false;
        for (const auto& existing : result) {
            duplicate = duplicate || existing == candidate;
        }
        if (!duplicate) {
            result.push_back(candidate);
        }
    }
    pending_releases_.clear();
    return result;
}

FixedList<OutputNote, ActiveNoteRegistry::Capacity> ActiveNoteRegistry::release(const VoiceId& owner) {
    pending_releases_.clear();
    for (auto& active : notes_) {
        if (active.active && active.owner == owner) {
            pending_releases_.push_back({active.output_channel, active.note});
            active.active = false;
        }
    }
    return collectReleased();
}

FixedList<OutputNote, ActiveNoteRegistry::Capacity> ActiveNoteRegistry::release(StreamId stream) {
    pending_releases_.clear();
    for (auto& active : notes_) {
        if (active.active && active.stream == stream) {
            pending_releases_.push_back({active.output_channel, active.note});
            active.active = false;
        }
    }
    return collectReleased();
}

FixedList<OutputNote, ActiveNoteRegistry::Capacity> ActiveNoteRegistry::releaseExcept(const VoiceId& owner, StreamId stream, uint8_t channel, const NoteList& desired) {
    pending_releases_.clear();
    for (auto& active : notes_) {
        if (!active.active || active.owner != owner || active.stream != stream) continue;
        bool keep = active.output_channel == channel;
        if (keep) {
            keep = false;
            for (const uint8_t note : desired) keep = keep || note == active.note;
        }
        if (!keep) {
            pending_releases_.push_back({active.output_channel, active.note});
            active.active = false;
        }
    }
    return collectReleased();
}

FixedList<OutputNote, ActiveNoteRegistry::Capacity> ActiveNoteRegistry::releaseAll() {
    pending_releases_.clear();
    for (auto& active : notes_) {
        if (active.active) {
            pending_releases_.push_back({active.output_channel, active.note});
            active.active = false;
        }
    }
    return collectReleased();
}

std::size_t ActiveNoteRegistry::size() const {
    std::size_t count = 0;
    for (const auto& active : notes_) {
        count += active.active ? 1U : 0U;
    }
    return count;
}

bool ActiveNoteRegistry::empty() const { return size() == 0; }

}
