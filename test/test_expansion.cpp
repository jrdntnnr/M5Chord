#include "TestSupport.h"
#include "app/App.h"
#include "controller/KeyBindings.h"
#include "controller/Parameters.h"
#include "storage/ProfileCodec.h"
#include "storage/StateCodec.h"
#include "transport/DinMidiSink.h"
#include "common/DiagnosticRing.h"
#include "hardware/InputKeys.h"

using namespace midibrain;

namespace {
void captureAction(void* context, const ActionEvent& action, uint64_t) {
    static_cast<FixedList<ActionEvent,64>*>(context)->push_back(action);
}
bool contains(const NoteList& notes, uint8_t pitch) {
    for (const auto note : notes) if (note == pitch) return true;
    return false;
}
}

void testExpansion() {
    {
        test::FakeSink sink;
        App app(sink);
        app.apply({SemanticAction::KeyLearn,1,true},1);
        EXPECT(app.captureKeySelection(MidiEvent::noteOn(9,61,100,2)));
        EXPECT(!app.keyLearning());
        EXPECT(app.state().harmonic.key_root == 1);
        EXPECT(app.captureKeySelection(MidiEvent::noteOn(9,61,100,3)));
        EXPECT(app.captureKeySelection(MidiEvent::noteOff(9,61,0,4)));
        EXPECT(!app.captureKeySelection(MidiEvent::noteOff(9,61,0,5)));
        EXPECT(sink.events.empty());
        app.receive(MidiEvent::noteOn(9,61,100,6));
        app.receive(MidiEvent::noteOff(9,61,0,7));
        EXPECT(app.activeNotes().empty());
        app.state().mode = EngineMode::Chord;
        app.state().performance.mode = PerformanceMode::Arp;
        app.receive(MidiEvent::noteOn(0,60,100,10));
        app.tick(10);
        sink.events.clear();
        app.tick(1000000000000ULL);
        EXPECT(sink.events.size() <= 3);
        app.receive(MidiEvent::noteOff(0,60,0,1000000000001ULL));
        EXPECT(app.scheduler().empty());
        EXPECT(app.activeNotes().empty());
    }
    {
        test::FakeSink sink;
        App app(sink);
        app.state().mode = EngineMode::Chord;
        app.state().harmonic.play_style = PlayStyle::Free;
        app.state().velocity_sensitive = false;
        app.receive(MidiEvent::noteOn(0,60,12,1));
        app.tick(1);
        EXPECT(app.lastChord().size() == 1);
        EXPECT(sink.events[0].data2 == 100);
        app.apply({SemanticAction::ChordMaj,1,true},2);
        app.tick(2);
        EXPECT(app.lastChord().size() == 3);
        app.apply({SemanticAction::ChordMaj,0,false},3);
        const auto count = sink.events.size();
        app.apply({SemanticAction::ChordMaj,1,true},4);
        app.tick(4);
        EXPECT(sink.events.size() == count + 6);
        app.apply({SemanticAction::ChordMin,1,true},5);
        app.tick(5);
        EXPECT(contains(app.lastChord(),63));
        app.panic(6);
    }
    {
        DiagnosticRing ring;
        for (unsigned i=0;i<100;++i) ring.push({i});
        EXPECT(ring.size() == 64);
        EXPECT(ring.at(0).at_us == 36);
        EXPECT(ring.at(63).at_us == 99);
    }
    {
        test::FakeSink sink;
        App app(sink);
        ControllerMapper mapper;
        ProfileStore profiles;
        FixedList<ActionEvent,64> actions;
        InputKeys input(app,mapper,profiles,captureAction,&actions);
        EXPECT(input.handleAction({SemanticAction::OptionsToggle,1,true},1));
        EXPECT(input.menuOpen());
        input.handleAction({SemanticAction::MenuDecrease,1,true},2);
        EXPECT(actions.size() == 1 && actions[0].value == -1);
        actions.clear();
        for (std::size_t i=0;i<std::size(parameters);++i) input.handleAction({SemanticAction::MenuDown,1,true},3+i);
        input.handleAction({SemanticAction::MenuIncrease,1,true},100);
        EXPECT(actions.empty());
        input.handleAction({SemanticAction::MenuConfirm,1,true},101);
        EXPECT(actions.size() == 1 && actions[0].action == SemanticAction::KeyLearn);
        input.handleAction({SemanticAction::LearnControl,1,true},102);
        EXPECT(input.captureLearnEvent(MidiEvent::cc(3,21,100,103)));
        char title[80]{};
        char value[96]{};
        input.menuText(title,sizeof(title),value,sizeof(value));
        EXPECT(input.menuOpen());
        EXPECT(std::string(title).find("Action") != std::string::npos);
        for (int i=0;i<4;++i) input.handleAction({SemanticAction::MenuDown,1,true},104+i);
        input.handleAction({SemanticAction::MenuConfirm,1,true},110);
        EXPECT(mapper.size() == 1);
        EXPECT(mapper.mapping(0).source.number == 21);
        EXPECT(mapper.mapping(0).source.channel == 3);
        input.handleAction({SemanticAction::MappingEdit,1,true},111);
        input.handleAction({SemanticAction::MenuIncrease,1,true},112);
        for (int i=0;i<4;++i) input.handleAction({SemanticAction::MenuDown,1,true},113+i);
        input.handleAction({SemanticAction::MenuConfirm,1,true},120);
        EXPECT(mapper.size() == 1);
        EXPECT(mapper.mapping(0).action == SemanticAction::VoicingDown);
        input.handleAction({SemanticAction::MappingDelete,1,true},121);
        EXPECT(mapper.size() == 0);
    }
    {
        KeyDispatcher keyboard;
        FixedList<ActionEvent,64> actions;
        std::array<bool,128> keys{};
        keys['s'] = true;
        keyboard.update(keys,0,false,1,captureAction,&actions);
        keys['d'] = true;
        keyboard.update(keys,0,false,2,captureAction,&actions);
        EXPECT(actions.size() == 2);
        EXPECT(actions[0].action == SemanticAction::ExtensionMinor7);
        keys['s'] = false;
        keyboard.update(keys,1,true,3,captureAction,&actions);
        EXPECT(actions.size() == 3);
        EXPECT(!actions[2].pressed);
        EXPECT(actions[2].action == SemanticAction::ExtensionMinor7);
        keys.fill(false);
        keyboard.update(keys,1,true,4,captureAction,&actions);
        actions.clear();
        keys['s'] = true;
        keyboard.update(keys,1,false,5,captureAction,&actions);
        EXPECT(actions.size() == 1);
        EXPECT(actions[0].action == SemanticAction::PresetSave);
        keyboard.update(keys,0,false,6,captureAction,&actions);
        EXPECT(actions.size() == 1);
        keys.fill(false);
        keyboard.update(keys,0,false,7,captureAction,&actions);
        keys['l'] = true;
        keyboard.update(keys,0,true,8,captureAction,&actions);
        EXPECT(actions.size() == 1);
        EXPECT(keyBindingsUnique());
    }
    {
        test::FakeSink sink;
        App app(sink);
        app.state().mode = EngineMode::Chord;
        app.apply({SemanticAction::Extension9,1,true,102},1);
        app.apply({SemanticAction::Extension9,1,true,128},2);
        app.apply({SemanticAction::Extension9,0,false,102},3);
        EXPECT(app.state().harmonic.extensions == Extension9);
        app.apply({SemanticAction::Extension9,0,false,128},4);
        EXPECT(app.state().harmonic.extensions == 0);
        app.apply({SemanticAction::ModeNext,0,false},5);
        EXPECT(app.state().mode == EngineMode::Chord);
        adjustParameter(app,1,-1,6);
        EXPECT(app.state().harmonic.play_style == PlayStyle::Free);
        adjustParameter(app,1,-1,6);
        EXPECT(app.state().harmonic.play_style == PlayStyle::Advanced);
        adjustParameter(app,1,-1,7);
        EXPECT(app.state().harmonic.play_style == PlayStyle::Simple);
        app.receive(MidiEvent::noteOn(0,60,100,10));
        app.tick(10);
        EXPECT(app.lastChord().size() == 1);
        app.apply({SemanticAction::ChordMin,1,true},11);
        EXPECT(app.lastChord().size() == 1);
        app.receive(MidiEvent::noteOff(0,60,0,12));
        app.receive(MidiEvent::noteOn(0,60,100,13));
        app.tick(13);
        EXPECT(contains(app.lastChord(),63));
        app.apply({SemanticAction::ChordMaj,1,true},14);
        app.apply({SemanticAction::VoicingUp,1,true},15);
        EXPECT(contains(app.lastChord(),63));
        EXPECT(!contains(app.lastChord(),64));
        app.panic(16);
        EXPECT(app.activeNotes().size() == 0);
    }
    {
        test::FakeSink sink;
        App app(sink);
        app.state().mode = EngineMode::Chord;
        app.state().harmonic.play_style = PlayStyle::Advanced;
        app.receive(MidiEvent::noteOn(0,60,100,1));
        app.tick(1);
        EXPECT(app.lastChord().size() == 1);
        app.apply({SemanticAction::ChordMin,1,true},2);
        app.tick(2);
        EXPECT(contains(app.lastChord(),63));
        app.apply({SemanticAction::ChordMaj,1,true},3);
        app.apply({SemanticAction::Extension9,1,true},4);
        app.tick(4);
        EXPECT(contains(app.lastChord(),63));
        EXPECT(!contains(app.lastChord(),64));
        EXPECT(contains(app.lastChord(),74));
        app.apply({SemanticAction::PerformanceChannelSet,4,true},5);
        EXPECT(app.activeNotes().size() == 0);
        EXPECT(app.scheduler().empty());
        EXPECT(app.state().routing.performance_channel == 3);
        app.receive(MidiEvent::noteOff(0,60,0,6));
        EXPECT(app.activeNotes().size() == 0);
    }
    {
        AppState original;
        original.mode = EngineMode::Key;
        original.harmonic.play_style = PlayStyle::Advanced;
        original.harmonic.transpose = -12;
        original.routing.performance_channel = 3;
        original.routing.bass_channel = 8;
        original.loop_bars = 8;
        original.loop_quantize = 4;
        original.preset_slot = 15;
        original.harmonic.extensions = Extension9;
        JsonDocument document;
        StateCodec::encode(original,document);
        AppState restored;
        EXPECT(StateCodec::decode(document,restored));
        EXPECT(StateCodec::fingerprint(original) == StateCodec::fingerprint(restored));
        EXPECT(restored.harmonic.extensions == 0);
        EXPECT(restored.routing.performance_channel == 3);
        EXPECT(restored.preset_slot == 15);
        const auto fingerprint = StateCodec::fingerprint(restored);
        document["performance_ch"] = 999;
        EXPECT(!StateCodec::decode(document,restored));
        EXPECT(StateCodec::fingerprint(restored) == fingerprint);
        StateCodec::encode(original,document);
        document["bpm"] = 120;
        EXPECT(!StateCodec::decode(document,restored));
        original.harmonic.extension_stack = true;
        StateCodec::encode(original,document);
        EXPECT(StateCodec::decode(document,restored));
        EXPECT(restored.harmonic.extensions == Extension9);
    }
    {
        ControllerProfile original;
        ControllerMapping mapping;
        mapping.source = {MidiType::ControlChange,9,20,10,127,2};
        mapping.action = SemanticAction::VoicingDelta;
        mapping.trigger = MappingTrigger::Relative;
        mapping.relative_mode = RelativeMode::BinaryOffset;
        mapping.consume = true;
        original.mapper.add(mapping);
        original.input_channel = 4;
        original.low = 36;
        original.high = 96;
        original.vid = 123;
        original.pid = 456;
        JsonDocument document;
        ProfileCodec::encode(original,document);
        ControllerProfile restored;
        const char* error = nullptr;
        EXPECT(ProfileCodec::decode(document,restored,error));
        EXPECT(restored.mapper.size() == 1);
        EXPECT(restored.mapper.mapping(0).source.cable == 2);
        EXPECT(restored.mapper.mapping(0).relative_mode == RelativeMode::BinaryOffset);
        EXPECT(restored.input_channel == 4);
        EXPECT(ProfileCodec::match(restored,123,456,"","") == 30);
        document["mappings"][0]["source"]["channel"] = 257;
        EXPECT(!ProfileCodec::decode(document,restored,error));
        EXPECT(restored.mapper.size() == 1);
        ProfileCodec::encode(original,document);
        document["mappings"][0]["action"] = "invalid";
        EXPECT(!ProfileCodec::decode(document,restored,error));
    }
    {
        DinMidiSink sink;
        for (int i=0;i<200;++i) EXPECT(sink.send(MidiEvent::cc(0,1,i%128)));
        EXPECT(sink.queued() == 1);
        EXPECT(sink.send(MidiEvent::noteOn(0,60,100)));
        EXPECT(sink.send(MidiEvent::cc(0,1,5)));
        EXPECT(sink.queued() == 3);
        sink.discardPending();
        for (int i=0;i<384;++i) EXPECT(sink.send(MidiEvent::noteOff(0,i%128)));
        EXPECT(!sink.send(MidiEvent::noteOff(0,60)));
        EXPECT(sink.takeRecovery());
        EXPECT(sink.queued() == 0);
        EXPECT(!sink.takeRecovery());
        App app(sink);
        app.panic(10);
        EXPECT(sink.queued() == 64);
    }
}
