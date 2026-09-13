#include "app/App.h"

#include <algorithm>

namespace midibrain {

App::App(MidiSink& sink) : sink_(sink) {}

bool App::hasHeldRoots() const {
    for (const auto& voice : voices_) if (voice.active) return true;
    return false;
}

uint8_t App::heldQualities() const { return quality_held_; }

const AppState& App::state() const { return state_; }
AppState& App::state() { return state_; }
const ActiveNoteRegistry& App::activeNotes() const { return active_notes_; }
const MidiScheduler& App::scheduler() const { return scheduler_; }
const NoteList& App::lastChord() const { return last_chord_; }
uint8_t App::lastRoot() const { return last_root_; }
MidiLooper& App::looper() { return looper_; }
const MidiLooper& App::looper() const { return looper_; }
bool App::keyLearning() const { return key_learning_; }
uint64_t App::lastActivity() const { return last_activity_us_; }
bool App::idle() const {
    for (const auto& voice : voices_) if (voice.active) return false;
    return looper_.mode() == LoopMode::Stopped && scheduler_.empty();
}

void App::stopLoop(uint64_t nowUs) {
    looper_.stop(nowUs);
    scheduler_.cancel(StreamId::Loop);
    const bool wasPlayback = playback_dispatch_;
    playback_dispatch_ = true;
    sendReleased(active_notes_.release(StreamId::Loop), nowUs);
    for (uint8_t channel = 0; channel < 16; ++channel) if (looper_.sustainChannels() & (1U << channel)) send(MidiEvent::cc(channel, 64, 0, nowUs));
    playback_dispatch_ = wasPlayback;
}

void App::changeRouting(uint8_t& channel, int value, uint64_t nowUs) {
    const uint8_t next = static_cast<uint8_t>(std::clamp(value, 1, 16) - 1);
    if (channel == next) return;
    panic(nowUs);
    channel = next;
}

bool App::acceptsRoot(const MidiEvent& event) const {
    const bool channelAccepted = state_.input_channel < 0 || state_.input_channel == event.channel;
    return channelAccepted && event.data1 >= state_.root_input_low && event.data1 <= state_.root_input_high;
}

void App::send(const MidiEvent& event) {
    if (sink_.send(event)) {
        output_activity_.observe(event, std::max(last_tick_us_, event.timestamp_us));
        ++state_.stats.midi_events_tx;
        if (!playback_dispatch_) looper_.capture(event, event.timestamp_us);
    } else {
        ++state_.stats.midi_events_dropped;
    }
}

void App::receive(const MidiEvent& incoming) {
    MidiEvent event = incoming;
    if (incoming.type == MidiType::NoteOn || incoming.type == MidiType::NoteOff) last_activity_us_ = incoming.timestamp_us;
    ++state_.stats.midi_events_rx;
    if (event.type == MidiType::NoteOn && event.data2 == 0) {
        event.type = MidiType::NoteOff;
    }
    if (captureKeySelection(event)) return;
    if (event.type == MidiType::NoteOn || event.type == MidiType::NoteOff) {
        if (event.type == MidiType::NoteOn) {
            startVoice(event);
        } else {
            releaseVoices(event.channel, event.data1, event.timestamp_us);
        }
        return;
    }
    if (state_.mode == EngineMode::Bypass) {
        if (state_.routing.primary_channel_override && event.type <= MidiType::PitchBend) event.channel = state_.routing.performance_channel;
        send(event);
        return;
    }
    if (event.type > MidiType::PitchBend) { send(event); return; }
    routeExpression(event);
}

bool App::captureKeySelection(const MidiEvent& event) {
    const bool release = event.type == MidiType::NoteOff || (event.type == MidiType::NoteOn && !event.data2);
    if (key_release_pending_ && (release || event.type == MidiType::NoteOn)
        && event.channel == learned_key_channel_ && event.data1 == learned_key_note_) {
        if (release) key_release_pending_ = false;
        return true;
    }
    if (!key_learning_ || event.type != MidiType::NoteOn || !event.data2) return false;
    state_.harmonic.key_root = event.data1 % 12;
    learned_key_channel_ = event.channel;
    learned_key_note_ = event.data1;
    key_release_pending_ = true;
    key_learning_ = false;
    rebuildHeld(event.timestamp_us);
    return true;
}

App::SourceVoice* App::allocateVoice(const MidiEvent& event) {
    for (auto& voice : voices_) {
        if (!voice.active) {
            voice = {};
            voice.active = true;
            voice.id = {event.channel, event.data1, ++generation_};
            voice.velocity = event.data2;
            return &voice;
        }
    }
    return nullptr;
}

NoteList App::buildNotes(uint8_t root, ChordQuality quality, uint8_t extensions) const {
    NoteList notes;
    if (state_.mode == EngineMode::Key && !quality_held_) {
        notes = scales_.buildDiatonic(root, state_.harmonic.key_root, state_.harmonic.scale, extensions);
    } else {
        notes = chords_.build(root, quality, extensions);
        if (state_.harmonic.harmonic_quantize) {
            notes = scales_.quantizeChord(notes, state_.harmonic.key_root, state_.harmonic.scale);
        }
    }
    notes = voicing_.apply(notes, state_.harmonic.voicing_step);
    for (auto& note : notes) note = static_cast<uint8_t>(std::clamp<int>(note + state_.harmonic.transpose, 0, 127));
    return notes;
}

void App::startVoice(const MidiEvent& event) {
    SourceVoice* voice = allocateVoice(event);
    if (voice == nullptr) {
        ++state_.stats.midi_events_dropped;
        return;
    }
    voice->quality = state_.harmonic.quality;
    voice->extensions = state_.harmonic.extensions;
    voice->bypass = state_.mode == EngineMode::Bypass || !acceptsRoot(event);
    if (!voice->bypass && !state_.velocity_sensitive) voice->velocity = 100;
    voice->chord = state_.mode == EngineMode::Key || state_.harmonic.play_style == PlayStyle::Latched || quality_held_;
    if (voice->bypass || !voice->chord) voice->notes.push_back(static_cast<uint8_t>(std::clamp<int>(event.data1 + (voice->bypass ? 0 : state_.harmonic.transpose), 0, 127)));
    else voice->notes = buildNotes(event.data1, voice->quality, voice->extensions);
    last_chord_ = voice->notes;
    last_root_ = event.data1;
    scheduleVoice(*voice, event.timestamp_us);
    if (voice->bypass) tick(event.timestamp_us);
}

void App::scheduleVoice(SourceVoice& voice, uint64_t startUs) {
    if (voice.bypass) {
        const uint8_t channel = state_.mode == EngineMode::Bypass && state_.routing.primary_channel_override ? state_.routing.performance_channel : voice.id.source_channel;
        scheduler_.schedule(startUs, voice.id, StreamId::Performance, MidiEvent::noteOn(channel, voice.notes[0], voice.velocity, startUs));
        return;
    }
    if (state_.routing.performance_enabled) {
        performance_.trigger(voice.notes, state_.routing.performance_channel, voice.velocity, startUs, voice.id, state_.performance, scheduler_);
        if (state_.performance.mode == PerformanceMode::Arp || state_.performance.mode == PerformanceMode::ArpTwoOctaves || state_.performance.mode == PerformanceMode::Pattern) {
            voice.next_step_us = startUs + performance_.stepDurationUs(state_.performance.bpm, state_.performance.division);
            voice.step = 1;
        }
    }
    scheduleRaw(voice, startUs);
    scheduleBass(voice, startUs);
}

void App::scheduleRaw(const SourceVoice& voice, uint64_t startUs) {
    if (!state_.routing.raw_chord_enabled) {
        return;
    }
    for (const uint8_t note : voice.notes) {
        scheduler_.schedule(startUs, voice.id, StreamId::RawChord, MidiEvent::noteOn(state_.routing.raw_chord_channel, note, voice.velocity, startUs));
    }
}

void App::scheduleBass(const SourceVoice& voice, uint64_t startUs) {
    for (const uint8_t note : bassNotes(voice)) {
        scheduler_.schedule(startUs, voice.id, StreamId::Bass, MidiEvent::noteOn(state_.routing.bass_channel, note, voice.velocity, startUs));
    }
}

NoteList App::bassNotes(const SourceVoice& voice) const {
    NoteList result;
    if (!state_.routing.bass_enabled || state_.bass.mode == BassMode::Off || voice.notes.empty()) return result;
    int note = voice.id.source_note;
    if (state_.bass.mode == BassMode::Lowest) note = voice.notes[0];
    else if (state_.bass.mode == BassMode::Root) {
        if (state_.mode == EngineMode::Key && !quality_held_) note = scales_.buildDiatonic(voice.id.source_note,state_.harmonic.key_root,state_.harmonic.scale)[0];
        note += state_.harmonic.transpose;
    }
    note += state_.bass.octave * 12;
    while (note < 0) note += 12;
    while (note > 127) note -= 12;
    result.push_back(static_cast<uint8_t>(note));
    return result;
}

void App::sendReleased(const FixedList<OutputNote, ActiveNoteRegistry::Capacity>& notes, uint64_t nowUs) {
    for (const auto& note : notes) {
        send(MidiEvent::noteOff(note.channel, note.note, 0, nowUs));
    }
}

void App::releaseVoice(SourceVoice& voice, uint64_t nowUs) {
    scheduler_.cancel(voice.id);
    sendReleased(active_notes_.release(voice.id), nowUs);
    voice = {};
}

void App::releaseVoices(uint8_t channel, uint8_t note, uint64_t nowUs) {
    for (auto& voice : voices_) {
        if (voice.active && voice.id.source_channel == channel && voice.id.source_note == note) {
            releaseVoice(voice, nowUs);
        }
    }
}

void App::dispatch(const ScheduledMidiEvent& scheduled) {
    bool shouldSend = false;
    if (scheduled.event.type == MidiType::NoteOn) {
        if (!active_notes_.activate(scheduled.owner, scheduled.stream, scheduled.event.channel, scheduled.event.data1, shouldSend)) {
            ++state_.stats.midi_events_dropped;
            return;
        }
        if (shouldSend) {
            send(scheduled.event);
            ++state_.stats.midi_events_generated;
        }
        return;
    }
    if (scheduled.event.type == MidiType::NoteOff) {
        active_notes_.deactivate(scheduled.owner, scheduled.stream, scheduled.event.channel, scheduled.event.data1, shouldSend);
        if (shouldSend) {
            send(scheduled.event);
            ++state_.stats.midi_events_generated;
        }
        return;
    }
    send(scheduled.event);
}

void App::tick(uint64_t nowUs) {
    nowUs = std::max(nowUs,last_tick_us_);
    last_tick_us_ = nowUs;
    if (looper_.tick(nowUs, scheduler_)) {
        playback_dispatch_ = true;
        sendReleased(active_notes_.release(StreamId::Loop), nowUs);
        for (uint8_t channel = 0; channel < 16; ++channel) if (looper_.sustainChannels() & (1U << channel)) send(MidiEvent::cc(channel, 64, 0, nowUs));
        playback_dispatch_ = false;
    }
    for (auto& voice : voices_) {
        if (!voice.active || voice.next_step_us == 0 || nowUs < voice.next_step_us) {
            continue;
        }
        NoteList pool = voice.notes;
        if (state_.performance.mode == PerformanceMode::ArpTwoOctaves) {
            pool = voicing_.expandOctaves(pool, 2);
        }
        const uint64_t duration = performance_.stepDurationUs(state_.performance.bpm,state_.performance.division);
        const uint64_t skipped = (nowUs-voice.next_step_us)/duration;
        voice.next_step_us += skipped*duration;
        voice.step += static_cast<uint32_t>(skipped);
        performance_.scheduleArpStep(pool, state_.routing.performance_channel, voice.velocity, voice.next_step_us, voice.step++, voice.id, state_.performance, scheduler_);
        voice.next_step_us += duration;
    }
    if (state_.clock.output_enabled) {
        const uint64_t interval = 60000000ULL / (static_cast<uint64_t>(state_.performance.bpm) * 24ULL);
        if (next_clock_us_ == 0) next_clock_us_ = nowUs;
        if (next_clock_us_ <= nowUs) {
            next_clock_us_ += ((nowUs-next_clock_us_)/interval)*interval;
            send({MidiType::Clock, 0, 0, 0, 0, next_clock_us_, 0});
            next_clock_us_ += interval;
        }
    }
    ScheduledMidiEvent event;
    while (scheduler_.popDue(nowUs, event)) {
        playback_dispatch_ = event.stream == StreamId::Loop;
        dispatch(event);
        playback_dispatch_ = false;
    }
}

void App::routeExpression(const MidiEvent& event) {
    if (state_.routing.expression_routing == ExpressionRouting::Disabled) {
        return;
    }
    if (state_.routing.expression_routing == ExpressionRouting::SourceOnly) {
        send(event);
        return;
    }
    FixedList<uint8_t, 3> channels;
    if (state_.routing.performance_enabled) channels.push_back(state_.routing.performance_channel);
    if (state_.routing.bass_enabled) channels.push_back(state_.routing.bass_channel);
    if (state_.routing.raw_chord_enabled) channels.push_back(state_.routing.raw_chord_channel);
    for (std::size_t i = 0; i < channels.size(); ++i) {
        bool duplicate = false;
        for (std::size_t j = 0; j < i; ++j) duplicate = duplicate || channels[i] == channels[j];
        if (!duplicate) {
            MidiEvent routed = event;
            routed.channel = channels[i];
            send(routed);
        }
    }
}

void App::rebuildHeld(uint64_t nowUs) {
    for (auto& voice : voices_) {
        if (!voice.active || voice.bypass) {
            continue;
        }
        scheduler_.cancel(voice.id);
        if (state_.harmonic.play_style == PlayStyle::Free || state_.harmonic.play_style == PlayStyle::Latched) voice.quality = state_.harmonic.quality;
        if (state_.harmonic.play_style != PlayStyle::Simple) voice.extensions = state_.harmonic.extensions;
        if (voice.chord) voice.notes = buildNotes(voice.id.source_note, voice.quality, voice.extensions);
        else voice.notes[0] = static_cast<uint8_t>(std::clamp<int>(voice.id.source_note + state_.harmonic.transpose, 0, 127));
        last_chord_ = voice.notes;
        if (state_.performance.mode == PerformanceMode::Block) {
            const NoteList empty;
            reconcileStream(voice, StreamId::Performance, state_.routing.performance_channel, state_.routing.performance_enabled ? voice.notes : empty, nowUs);
            reconcileStream(voice, StreamId::RawChord, state_.routing.raw_chord_channel, state_.routing.raw_chord_enabled ? voice.notes : empty, nowUs);
            reconcileStream(voice, StreamId::Bass, state_.routing.bass_channel, bassNotes(voice), nowUs);
        } else {
            sendReleased(active_notes_.release(voice.id), nowUs);
            scheduleVoice(voice, nowUs);
        }
    }
}

void App::rebuildHarmony(uint64_t nowUs, bool qualityChange) {
    if (state_.harmonic.play_style == PlayStyle::Simple) return;
    if (state_.harmonic.play_style == PlayStyle::Free && quality_held_) {
        for (auto& voice : voices_) if (voice.active && !voice.bypass) voice.chord = true;
    }
    if (state_.harmonic.play_style == PlayStyle::Advanced) {
        for (auto& voice : voices_) {
            if (!voice.active || voice.bypass || voice.chord || !quality_held_) continue;
            voice.chord = true;
            voice.quality = state_.harmonic.quality;
            voice.extensions = state_.harmonic.extensions;
            if (qualityChange) {
                scheduler_.cancel(voice.id);
                voice.notes = buildNotes(voice.id.source_note, voice.quality, voice.extensions);
                last_chord_ = voice.notes;
                sendReleased(active_notes_.release(voice.id), nowUs);
                scheduleVoice(voice, nowUs);
            }
        }
        if (qualityChange) return;
    }
    if (state_.harmonic.retrigger_extensions || (state_.harmonic.play_style == PlayStyle::Free && qualityChange)) {
        for (const auto& voice : voices_) {
            if (!voice.active || voice.bypass) continue;
            sendReleased(active_notes_.release(voice.id), nowUs);
        }
    }
    rebuildHeld(nowUs);
}

void App::reconcileStream(const SourceVoice& voice, StreamId stream, uint8_t channel, const NoteList& desired, uint64_t nowUs) {
    sendReleased(active_notes_.releaseExcept(voice.id, stream, channel, desired), nowUs);
    for (const uint8_t note : desired) {
        if (!active_notes_.owns(voice.id, stream, channel, note)) {
            scheduler_.schedule(nowUs, voice.id, stream, MidiEvent::noteOn(channel, note, voice.velocity, nowUs));
        }
    }
}

void App::setPerformance(PerformanceMode mode, uint64_t nowUs) {
    if (state_.performance.mode == mode) {
        return;
    }
    state_.performance.mode = mode;
    rebuildHeld(nowUs);
}

void App::apply(const ActionEvent& action, uint64_t nowUs) {
    const bool harmonicControl = action.action >= SemanticAction::ChordDim && action.action <= SemanticAction::Extension9;
    if (!action.pressed && !harmonicControl) return;
    if (harmonicControl && action.source && action.source < held_controls_.size()) {
        held_controls_[action.source] = action.pressed ? action.action : SemanticAction::None;
        if (!action.pressed) {
            for (const auto held : held_controls_) if (held == action.action) return;
        }
    }
    if (action.action >= SemanticAction::ChordDim && action.action <= SemanticAction::ChordSus) {
        const uint8_t bit = 1U << (static_cast<uint8_t>(action.action) - static_cast<uint8_t>(SemanticAction::ChordDim));
        if (action.pressed) quality_held_ |= bit;
        else {
            quality_held_ &= static_cast<uint8_t>(~bit);
            if (state_.mode == EngineMode::Key) rebuildHarmony(nowUs);
            return;
        }
    }
    const int direction = action.value < 0 ? -1 : 1;
    bool rebuild = false;
    switch (action.action) {
        case SemanticAction::VelocityToggle: state_.velocity_sensitive = !state_.velocity_sensitive; break;
        case SemanticAction::ChordDim: state_.harmonic.quality = ChordQuality::Diminished; rebuild = action.pressed; break;
        case SemanticAction::ChordMin: state_.harmonic.quality = ChordQuality::Minor; rebuild = action.pressed; break;
        case SemanticAction::ChordMaj: state_.harmonic.quality = ChordQuality::Major; rebuild = action.pressed; break;
        case SemanticAction::ChordSus: state_.harmonic.quality = ChordQuality::Suspended; rebuild = action.pressed; break;
        case SemanticAction::Extension6:
        case SemanticAction::ExtensionMinor7:
        case SemanticAction::ExtensionMajor7:
        case SemanticAction::Extension9: {
            const uint8_t bit = action.action == SemanticAction::Extension6 ? Extension6
                : action.action == SemanticAction::ExtensionMinor7 ? ExtensionMinor7
                : action.action == SemanticAction::ExtensionMajor7 ? ExtensionMajor7
                : Extension9;
            if (state_.harmonic.extension_stack) {
                if (action.pressed) state_.harmonic.extensions ^= bit;
                else break;
            } else if (action.pressed) {
                state_.harmonic.extensions |= bit;
            } else {
                state_.harmonic.extensions &= static_cast<uint8_t>(~bit);
            }
            rebuild = true;
            break;
        }
        case SemanticAction::ExtensionStackToggle:
            if (action.pressed) {
                state_.harmonic.extension_stack = !state_.harmonic.extension_stack;
                state_.harmonic.extensions = 0;
                rebuild = true;
            }
            break;
        case SemanticAction::ModeBypass: panic(nowUs); state_.mode = EngineMode::Bypass; break;
        case SemanticAction::ModeChord: panic(nowUs); state_.mode = EngineMode::Chord; break;
        case SemanticAction::ModeKey: panic(nowUs); state_.mode = EngineMode::Key; break;
        case SemanticAction::ModeNext: panic(nowUs); state_.mode = static_cast<EngineMode>((static_cast<uint8_t>(state_.mode) + 3 + direction) % 3); break;
        case SemanticAction::KeyToggle: panic(nowUs); state_.mode = state_.mode == EngineMode::Key ? EngineMode::Chord : EngineMode::Key; break;
        case SemanticAction::KeyRootSet: state_.harmonic.key_root = static_cast<uint8_t>((action.value % 12 + 12) % 12); rebuild = true; break;
        case SemanticAction::KeyLearn: key_learning_ = true; break;
        case SemanticAction::PlayStyleNext: panic(nowUs); state_.harmonic.play_style = static_cast<PlayStyle>((static_cast<uint8_t>(state_.harmonic.play_style) + 4 + direction) % 4); break;
        case SemanticAction::ExtensionAdditionToggle: state_.harmonic.retrigger_extensions = !state_.harmonic.retrigger_extensions; break;
        case SemanticAction::TransposeSet: state_.harmonic.transpose = std::clamp<int>(action.value, -24, 24); rebuild = true; break;
        case SemanticAction::PerformanceDirectionNext: state_.performance.direction = static_cast<Direction>((static_cast<uint8_t>(state_.performance.direction) + 4 + direction) % 4); rebuild = true; break;
        case SemanticAction::StrumIntervalSet: state_.performance.strum_interval_ms = std::clamp<int>(action.value, 2, 120); break;
        case SemanticAction::PerformanceChannelSet: changeRouting(state_.routing.performance_channel, action.value, nowUs); state_.routing.primary_channel_override = true; break;
        case SemanticAction::BassChannelSet: changeRouting(state_.routing.bass_channel, action.value, nowUs); break;
        case SemanticAction::RawChannelSet: changeRouting(state_.routing.raw_chord_channel, action.value, nowUs); break;
        case SemanticAction::ExpressionNext: panic(nowUs); state_.routing.expression_routing = static_cast<ExpressionRouting>((static_cast<uint8_t>(state_.routing.expression_routing) + 3 + direction) % 3); break;
        case SemanticAction::InputChannelSet: panic(nowUs); state_.input_channel = std::clamp<int>(action.value - 1, -1, 15); break;
        case SemanticAction::InputLowSet: panic(nowUs); state_.root_input_low = std::clamp<int>(action.value, 0, state_.root_input_high); break;
        case SemanticAction::InputHighSet: panic(nowUs); state_.root_input_high = std::clamp<int>(action.value, state_.root_input_low, 127); break;
        case SemanticAction::LoopRecord:
            if (looper_.mode() == LoopMode::Recording) looper_.play(nowUs);
            else {
                stopLoop(nowUs);
                constexpr uint16_t grids[]{0, 96, 48, 32, 24, 16, 12};
                looper_.record(nowUs, state_.performance.bpm, state_.loop_bars, grids[state_.loop_quantize]);
            }
            break;
        case SemanticAction::LoopPlay:
            if (looper_.mode() == LoopMode::Playing || looper_.mode() == LoopMode::Overdub) stopLoop(nowUs);
            else looper_.play(nowUs);
            break;
        case SemanticAction::LoopStop: stopLoop(nowUs); break;
        case SemanticAction::LoopClear: stopLoop(nowUs); looper_.clear(); break;
        case SemanticAction::LoopOverdub: looper_.overdub(nowUs); break;
        case SemanticAction::LoopUndo: stopLoop(nowUs); looper_.undo(nowUs); break;
        case SemanticAction::LoopLengthNext: state_.loop_bars = direction > 0 ? (state_.loop_bars == 0 ? 1 : state_.loop_bars == 16 ? 0 : state_.loop_bars * 2) : (state_.loop_bars == 0 ? 16 : state_.loop_bars == 1 ? 0 : state_.loop_bars / 2); break;
        case SemanticAction::LoopQuantizeNext: state_.loop_quantize = (state_.loop_quantize + 7 + direction) % 7; break;
        case SemanticAction::PresetNext: state_.preset_slot = (state_.preset_slot + 16 + direction) % 16; break;
        case SemanticAction::PresetPrev: state_.preset_slot = (state_.preset_slot + 15) % 16; break;
        case SemanticAction::KeyNext: state_.harmonic.key_root = (state_.harmonic.key_root + 1) % 12; rebuild = true; break;
        case SemanticAction::KeyPrev: state_.harmonic.key_root = (state_.harmonic.key_root + 11) % 12; rebuild = true; break;
        case SemanticAction::ScaleNext: state_.harmonic.scale = static_cast<ScaleType>((static_cast<uint8_t>(state_.harmonic.scale) + ScaleCount + direction) % ScaleCount); rebuild = true; break;
        case SemanticAction::ScalePrev: state_.harmonic.scale = static_cast<ScaleType>((static_cast<uint8_t>(state_.harmonic.scale) + ScaleCount - 1) % ScaleCount); rebuild = true; break;
        case SemanticAction::HarmonicQuantizeToggle: state_.harmonic.harmonic_quantize = !state_.harmonic.harmonic_quantize; rebuild = true; break;
        case SemanticAction::VoicingUp: state_.harmonic.voicing_step = std::min<int8_t>(8, state_.harmonic.voicing_step + 1); rebuild = true; break;
        case SemanticAction::VoicingDown: state_.harmonic.voicing_step = std::max<int8_t>(-8, state_.harmonic.voicing_step - 1); rebuild = true; break;
        case SemanticAction::VoicingDelta: state_.harmonic.voicing_step = std::clamp<int>(state_.harmonic.voicing_step + action.value, -8, 8); rebuild = true; break;
        case SemanticAction::VoicingSet: state_.harmonic.voicing_step = std::clamp<int>(action.value, -8, 8); rebuild = true; break;
        case SemanticAction::PerformanceBlock: setPerformance(PerformanceMode::Block, nowUs); break;
        case SemanticAction::PerformanceStrum: setPerformance(PerformanceMode::Strum, nowUs); break;
        case SemanticAction::PerformanceStrumTwo: setPerformance(PerformanceMode::StrumTwoOctaves, nowUs); break;
        case SemanticAction::PerformanceSlop: setPerformance(PerformanceMode::Slop, nowUs); break;
        case SemanticAction::PerformanceArp: setPerformance(PerformanceMode::Arp, nowUs); break;
        case SemanticAction::PerformanceArpTwo: setPerformance(PerformanceMode::ArpTwoOctaves, nowUs); break;
        case SemanticAction::PerformancePattern: setPerformance(PerformanceMode::Pattern, nowUs); break;
        case SemanticAction::PerformanceHarp: setPerformance(PerformanceMode::Harp, nowUs); break;
        case SemanticAction::PerformanceNext: setPerformance(static_cast<PerformanceMode>((static_cast<uint8_t>(state_.performance.mode) + 8 + direction) % 8), nowUs); break;
        case SemanticAction::PerformancePrev: setPerformance(static_cast<PerformanceMode>((static_cast<uint8_t>(state_.performance.mode) + 7) % 8), nowUs); break;
        case SemanticAction::PerformanceRateNext: state_.performance.division = static_cast<Division>((static_cast<uint8_t>(state_.performance.division) + 6 + direction) % 6); break;
        case SemanticAction::PerformanceRatePrev: state_.performance.division = static_cast<Division>((static_cast<uint8_t>(state_.performance.division) + 5) % 6); break;
        case SemanticAction::PerformanceGateSet: state_.performance.gate_percent = std::clamp<int>(action.value, 1, 100); break;
        case SemanticAction::PerformanceAmountSet: state_.performance.slop_amount = std::clamp<int>(action.value, 0, 100); break;
        case SemanticAction::TempoSet: state_.performance.bpm = std::clamp<int>(action.value, 30, 300); break;
        case SemanticAction::TempoUp: state_.performance.bpm = std::min<uint16_t>(300, state_.performance.bpm + 1); break;
        case SemanticAction::TempoDown: state_.performance.bpm = std::max<uint16_t>(30, state_.performance.bpm - 1); break;
        case SemanticAction::TempoTap:
            if (tap_count_ > 0 && nowUs - tap_times_[(tap_count_ - 1) % 4] > 2000000ULL) tap_count_ = 0;
            tap_times_[tap_count_ % 4] = nowUs;
            ++tap_count_;
            if (tap_count_ >= 4) {
                const uint64_t span = tap_times_[(tap_count_ - 1) % 4] - tap_times_[(tap_count_ - 4) % 4];
                if (span > 0) state_.performance.bpm = std::clamp<uint64_t>(180000000ULL / span, 30, 300);
            }
            break;
        case SemanticAction::ClockToggle:
            state_.clock.output_enabled = !state_.clock.output_enabled;
            next_clock_us_ = 0;
            send({state_.clock.output_enabled ? MidiType::Start : MidiType::Stop, 0, 0, 0, 0, nowUs, 0});
            break;
        case SemanticAction::TransportStart: state_.clock.running = true; send({MidiType::Start, 0, 0, 0, 0, nowUs, 0}); break;
        case SemanticAction::TransportStop: state_.clock.running = false; send({MidiType::Stop, 0, 0, 0, 0, nowUs, 0}); break;
        case SemanticAction::TransportContinue: state_.clock.running = true; send({MidiType::Continue, 0, 0, 0, 0, nowUs, 0}); break;
        case SemanticAction::BassToggle: state_.routing.bass_enabled = !state_.routing.bass_enabled; state_.bass.mode = state_.routing.bass_enabled ? BassMode::Root : BassMode::Off; rebuild = true; break;
        case SemanticAction::BassModeNext: state_.bass.mode = static_cast<BassMode>((static_cast<uint8_t>(state_.bass.mode) + 4 + direction) % 4); state_.routing.bass_enabled = state_.bass.mode != BassMode::Off; rebuild = true; break;
        case SemanticAction::BassOctaveUp: state_.bass.octave = std::clamp<int>(state_.bass.octave + direction, -2, 1); rebuild = true; break;
        case SemanticAction::BassOctaveDown: state_.bass.octave = std::max<int8_t>(-2, state_.bass.octave - 1); rebuild = true; break;
        case SemanticAction::StreamPerformanceToggle: state_.routing.performance_enabled = !state_.routing.performance_enabled; rebuild = true; break;
        case SemanticAction::StreamBassToggle: state_.routing.bass_enabled = !state_.routing.bass_enabled; rebuild = true; break;
        case SemanticAction::StreamRawChordToggle: state_.routing.raw_chord_enabled = !state_.routing.raw_chord_enabled; rebuild = true; break;
        case SemanticAction::OutputLaneNext:
            if (action.pressed) {
                panic(nowUs);
                state_.routing.performance_channel = static_cast<uint8_t>((state_.routing.performance_channel + 1) % 4);
                state_.routing.primary_channel_override = true;
            }
            break;
        case SemanticAction::Panic: panic(nowUs); break;
        default: break;
    }
    if (rebuild && harmonicControl) rebuildHarmony(nowUs, action.action <= SemanticAction::ChordSus);
    else if (rebuild) rebuildHeld(nowUs);
}

void App::panic(uint64_t nowUs) {
    last_chord_.clear();
    looper_.stop(nowUs);
    quality_held_ = 0;
    held_controls_.fill(SemanticAction::None);
    key_learning_ = false;
    if (!state_.harmonic.extension_stack) state_.harmonic.extensions = 0;
    scheduler_.clear();
    sink_.discardPending();
    for (uint8_t channel = 0; channel < 16; ++channel) {
        send(MidiEvent::cc(channel, 64, 0, nowUs));
        send(MidiEvent::cc(channel, 120, 0, nowUs));
        send(MidiEvent::cc(channel, 121, 0, nowUs));
        send(MidiEvent::cc(channel, 123, 0, nowUs));
    }
    sendReleased(active_notes_.releaseAll(), nowUs);
    output_activity_.clear();
    for (auto& voice : voices_) voice = {};
    ++state_.stats.panic_count;
}

void App::onUsbDisconnected(uint64_t nowUs) { panic(nowUs); }

}
