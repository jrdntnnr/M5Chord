#ifdef ARDUINO

#include "app/App.h"
#include "common/DiagnosticRing.h"
#include "common/AppInfo.h"
#include "controller/ControllerMapper.h"
#include "hardware/InputKeys.h"
#include "hardware/CardputerHardware.h"
#include "storage/ProfileStore.h"
#include "storage/SettingsStore.h"
#include "storage/SessionStore.h"
#include "storage/MidiFileStore.h"
#include "storage/AtomicFile.h"
#include "storage/SdPaths.h"
#include "transport/BleMidiSource.h"
#include "transport/DinMidiSink.h"
#include "transport/DinMidiSource.h"
#include "transport/InputSelection.h"
#include "transport/BleTraceConsole.h"
#include "transport/UsbMidiSource.h"
#include "ui/Ui.h"

#include <Arduino.h>
#include <M5Cardputer.h>
#include <esp_timer.h>
#include <cstdio>
#include <cstring>

using namespace midibrain;

namespace {

DinMidiSink dinMidi(Serial2);
App app(dinMidi);
ControllerMapper controllerMapper;
ProfileStore profileStore;
SettingsStore settingsStore;
SessionStore sessionStore;
MidiFileStore midiFileStore;
char pendingMidiName[64]{};
Ui ui;
DiagnosticRing diagnostics;
uint64_t nextDiagnosticUs{0};
bool hardwareReady{false};
char lastInputStatus[96]{};
SemanticAction pendingStorage{SemanticAction::None};
AppState pendingPreset{};
uint8_t pendingSlot{0};
bool profileResolvePending{false};
bool bleReconnectPending{false};
InputSelection inputSelection;
uint8_t configuredInput{0};
InputPort usbPort{InputPort::Usb};
InputPort blePort{InputPort::Ble};
InputPort dinPort{InputPort::Din};
uint32_t savedSettingsFingerprint{0};
uint32_t pendingSettingsFingerprint{0};
uint64_t settingsSaveDueUs{0};
DisplayView savedView{DisplayView::Keyboard}, pendingView{DisplayView::Keyboard};
uint64_t viewSaveDueUs{0};

void handleAction(void*, const ActionEvent& action, uint64_t nowUs);
InputKeys inputKeys(app, controllerMapper, profileStore, handleAction, nullptr);

void handleAction(void*, const ActionEvent& action, uint64_t nowUs) {
    if (inputKeys.handleAction(action, nowUs)) return;
    if ((midiFileStore.loading() || pendingStorage == SemanticAction::MidiFileLoad) && (action.action == SemanticAction::MidiFileLoad || action.action == SemanticAction::MidiFileBrowse || action.action == SemanticAction::MidiFilePlay)) return;
    if (action.action == SemanticAction::BleReconnect) {
        if (action.pressed) { app.panic(nowUs); controllerMapper.resetEdges(); bleReconnectPending = true; }
    } else if (action.action == SemanticAction::ViewNext) {
        if (action.pressed) ui.nextView();
    } else if (action.action == SemanticAction::ViewPrev) {
        if (action.pressed) { ui.nextView(); ui.nextView(); ui.nextView(); }
    } else if (action.action == SemanticAction::PresetSave || action.action == SemanticAction::PresetLoad
        || action.action == SemanticAction::LoopSave || action.action == SemanticAction::LoopLoad
        || action.action == SemanticAction::DiagnosticsExport || action.action == SemanticAction::MidiFileBrowse
        || action.action == SemanticAction::MidiFileLoad) {
        if (action.pressed) {
            if (pendingStorage != SemanticAction::None) { ui.showOverlay("STORAGE REQUEST PENDING",nowUs); return; }
            if (action.action == SemanticAction::PresetLoad || action.action == SemanticAction::LoopLoad) app.panic(nowUs);
            if (action.action == SemanticAction::MidiFileBrowse || action.action == SemanticAction::MidiFileLoad) app.panic(nowUs);
            if (action.action == SemanticAction::MidiFileLoad) {
                const auto& names = inputKeys.fileCatalog().names;
                if (action.value < 0 || static_cast<std::size_t>(action.value) >= names.size()) { ui.showOverlay("SELECT A MIDI FILE FIRST", nowUs); return; }
                std::snprintf(pendingMidiName, sizeof(pendingMidiName), "%s", names[action.value].data());
            }
            pendingPreset = app.state();
            pendingSlot = app.state().preset_slot;
            pendingStorage = action.action;
            ui.showOverlay("QUEUED: RELEASE / STOP LOOP",nowUs);
            return;
        }
    } else {
        app.apply(action, nowUs);
    }
    if (action.pressed && (action.action < SemanticAction::ChordDim || action.action > SemanticAction::Extension9)) {
        char message[96]{};
        const char* label = ControllerMapper::actionName(action.action);
        bool formatted = false;
        for (std::size_t i=0;i<std::size(parameters);++i) {
            if (parameters[i].action != action.action) continue;
            char value[40]{};
            formatParameter(i,app.state(),value,sizeof(value));
            std::snprintf(message,sizeof(message),"%s: %s",parameters[i].label,value);
            formatted = true;
            break;
        }
        ui.showOverlay(formatted ? message : label,nowUs);
    }
}

void handleMidi(void* context, const MidiEvent& event) {
    if (configuredInput != app.state().input_port) {
        configuredInput = app.state().input_port;
        inputSelection.configure(static_cast<InputPort>(configuredInput));
        controllerMapper.resetEdges();
    }
    const auto previous = inputSelection.active();
    if (!inputSelection.accept(*static_cast<InputPort*>(context), event)) return;
    if (inputSelection.active() != previous) profileResolvePending = true;
    if (inputKeys.captureLearnEvent(event) || app.captureKeySelection(event)) { ++app.state().stats.midi_events_rx; return; }
    MappingResult mapped = controllerMapper.map(event);
    for (const auto& action : mapped.actions) handleAction(nullptr, action, event.timestamp_us);
    if (!mapped.consumed) app.receive(event);
    else ++app.state().stats.midi_events_rx;
}

void handleConnection(void* context, bool connected) {
    if (!connected && inputSelection.disconnected(*static_cast<InputPort*>(context))) { app.onUsbDisconnected(esp_timer_get_time()); controllerMapper.resetEdges(); }
    else profileResolvePending = true;
}

void handleDinRecovery(void* context) { handleConnection(context, false); }

UsbMidiSource usbMidi(handleMidi, handleConnection, &usbPort);
BleMidiSource bleMidi(handleMidi, handleConnection, &blePort);
DinMidiSource dinInput(handleMidi, handleDinRecovery, &dinPort);

#ifdef M5CHORD_USB_DIAGNOSTICS
void serviceConsole(uint64_t nowUs) {
    static BleTraceConsole trace;
    static char output[256]{};
    static std::size_t length{0};
    static std::size_t sent{0};
    static uint64_t summaryDue{0};
    static bool banner{false};
    static bool fileSummary{false};
    for (unsigned i = 0; i < 16 && Serial.available(); ++i) {
        const int command = Serial.read();
        if (command == 'r') handleAction(nullptr, {SemanticAction::BleReconnect, 1, true}, nowUs);
        else if (command == 'p') handleAction(nullptr, {SemanticAction::Panic, 1, true}, nowUs);
        else if (command == 'l') trace.rewind(bleMidi.trace());
    }
    if (!Serial || Serial.availableForWrite() <= 0) return;
    if (sent == length) {
        if (!banner) {
            std::snprintf(output, sizeof(output), "%s %s DIAGNOSTIC USB_HOST=OFF commands=r:reconnect l:trace p:panic\n", AppName, AppVersion);
            banner = true;
        } else if (fileSummary) {
            fileSummary = false;
            std::snprintf(output, sizeof(output), "FILE name=%.63s state=%.32s checking=%u progress=%u elapsed_us=%llu duration_us=%llu\n",
                app.midiFile().name(), app.midiFile().status(), midiFileStore.loading(), midiFileStore.progress(),
                static_cast<unsigned long long>(app.midiFile().elapsed(nowUs)), static_cast<unsigned long long>(app.midiFile().file().duration_us));
        } else if (!trace.next(bleMidi.trace(), output, sizeof(output))) {
            if (nowUs < summaryDue) return;
            summaryDue = nowUs + 1000000;
            fileSummary = true;
            const auto& d = bleMidi.diagnostics();
            std::snprintf(output, sizeof(output), "STATUS us=%llu bt=%s product=%.32s N=%lu E=%lu din=%lu input=%u rx=%lu tx=%lu errors=%lu trace_drop=%lu heap=%lu stack=%u\n",
                static_cast<unsigned long long>(nowUs), BleMidiSource::stateName(d.state), d.product,
                static_cast<unsigned long>(d.notifications_received), static_cast<unsigned long>(d.events_received),
                static_cast<unsigned long>(dinInput.events()), static_cast<unsigned>(inputSelection.active()),
                static_cast<unsigned long>(app.state().stats.midi_events_rx), static_cast<unsigned long>(app.state().stats.midi_events_tx),
                static_cast<unsigned long>(d.errors), static_cast<unsigned long>(d.trace_dropped),
                static_cast<unsigned long>(ESP.getFreeHeap()), static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
        }
        sent = 0;
        length = std::strlen(output);
    }
    const auto available = static_cast<std::size_t>(Serial.availableForWrite());
    sent += Serial.write(reinterpret_cast<const uint8_t*>(output) + sent, std::min(available, length - sent));
}
#endif

}

void setup() {
#ifdef M5CHORD_USB_DIAGNOSTICS
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
#endif
    auto configuration = M5.config();
    configuration.internal_spk = false;
    configuration.internal_mic = false;
    configuration.internal_imu = false;
    M5Cardputer.begin(configuration, true);
    ui.begin();
    if (detectedCardputerFamily() == CardputerFamily::Unsupported) {
        ui.showBootStatus("UNSUPPORTED BOARD");
        return;
    }
    ui.showBootStatus("MIDI OUTPUT");
    dinMidi.begin();
    dinInput.begin(Serial2);
    ui.showBootStatus("SETTINGS");
    settingsStore.load(app.state());
    ui.setView(settingsStore.loadView());
    savedView = ui.view(); pendingView = savedView;
    configuredInput = app.state().input_port;
    inputSelection.configure(static_cast<InputPort>(configuredInput));
    savedSettingsFingerprint = settingsStore.fingerprint(app.state());
    pendingSettingsFingerprint = savedSettingsFingerprint;
    ui.showBootStatus("SD CARD");
    profileStore.begin();
    ui.showBootStatus("CONTROLLER PROFILE");
    profileStore.loadActive(controllerMapper);
    profileStore.applyInput(app.state());
#ifndef M5CHORD_USB_DIAGNOSTICS
    ui.showBootStatus("USB HOST");
    if (!usbMidi.begin()) {
        ui.showOverlay("USB INIT FAILED", esp_timer_get_time());
    }
#endif
    ui.showBootStatus("BLE MIDI");
    if (!bleMidi.begin()) {
        ui.showOverlay("BLE INIT FAILED", esp_timer_get_time());
    }
    hardwareReady = true;
}

void loop() {
    if (!hardwareReady) { vTaskDelay(1); return; }
    uint64_t nowUs = esp_timer_get_time();
    M5Cardputer.update();
    inputKeys.poll(nowUs);
    if (configuredInput != app.state().input_port) {
        configuredInput = app.state().input_port;
        inputSelection.configure(static_cast<InputPort>(configuredInput));
        controllerMapper.resetEdges();
        profileResolvePending = true;
    }
#ifndef M5CHORD_USB_DIAGNOSTICS
    usbMidi.poll();
#endif
    dinInput.poll();
    if (bleReconnectPending) { bleMidi.requestReconnect(); bleReconnectPending = false; }
    bleMidi.poll(app.idle() && dinMidi.queued() == 0);
    nowUs = esp_timer_get_time();
    if (midiFileStore.complete(app.midiFile().file())) {
        app.midiFile().stop();
        if (inputKeys.playerOpen()) inputKeys.selectMidiPlayback();
    }
    inputKeys.setFileLoading(midiFileStore.loading(), midiFileStore.progress());
    app.tick(nowUs);
    if (dinMidi.takeRecovery()) app.panic(nowUs);
    dinMidi.poll();
    app.state().stats.din_events_rx = dinInput.events();
    app.state().stats.din_errors = dinInput.errors();
#ifdef M5CHORD_USB_DIAGNOSTICS
    serviceConsole(nowUs);
#endif
    if (nowUs >= nextDiagnosticUs) {
        nextDiagnosticUs = nowUs + 1000000;
        diagnostics.push({nowUs,app.state().stats.midi_events_rx,app.state().stats.midi_events_tx,
            app.state().stats.midi_events_dropped,app.scheduler().stats().max_late_us,
            bleMidi.diagnostics().events_dropped,dinMidi.dropped(),static_cast<uint16_t>(app.activeNotes().size()),
            static_cast<uint16_t>(dinMidi.queued()),static_cast<uint8_t>(app.looper().mode())});
    }
    if (app.idle() && dinMidi.queued() == 0) {
        inputKeys.service(nowUs);
        if (std::strcmp(lastInputStatus,inputKeys.status())) {
            std::snprintf(lastInputStatus,sizeof(lastInputStatus),"%s",inputKeys.status());
            ui.showOverlay(lastInputStatus,nowUs);
        }
        if (profileResolvePending) {
            profileResolvePending = false;
            const auto& usb = usbMidi.diagnostics();
            const bool fromUsb = inputSelection.active() == InputPort::Usb;
            const char* product = fromUsb ? usb.product : inputSelection.active() == InputPort::Din ? "DIN MIDI" : bleMidi.diagnostics().product;
            profileStore.resolve(fromUsb ? usb.vid : 0, fromUsb ? usb.pid : 0, fromUsb ? usb.manufacturer : "", product, controllerMapper);
            profileStore.applyInput(app.state());
        }
        if (pendingStorage != SemanticAction::None) {
            const auto action = pendingStorage;
            pendingStorage = SemanticAction::None;
            if (action == SemanticAction::PresetSave) sessionStore.savePreset(pendingPreset);
            else if (action == SemanticAction::PresetLoad) {
                AppState loaded = app.state();
                loaded.preset_slot = pendingSlot;
                if (sessionStore.loadPreset(loaded)) app.state() = loaded;
            }
            else if (action == SemanticAction::LoopSave) sessionStore.saveLoop(app.looper(),pendingSlot);
            else if (action == SemanticAction::LoopLoad) sessionStore.loadLoop(app.looper(),pendingSlot);
            else if (action == SemanticAction::MidiFileBrowse) {
                if (midiFileStore.scan(inputKeys.fileCatalog())) inputKeys.openFiles();
                ui.showOverlay(midiFileStore.status(), nowUs);
            } else if (action == SemanticAction::MidiFileLoad) {
                app.midiFile().file().events.clear();
                app.midiFile().file().duration_us = 0;
                app.midiFile().file().channels = 0;
                app.midiFile().file().error = "LOADING";
                app.midiFile().setName(pendingMidiName);
                app.midiFile().attach(&midiFileStore.stream());
                if (!midiFileStore.beginLoad(pendingMidiName)) app.midiFile().file().error = midiFileStore.status();
                app.midiFile().stop();
                inputKeys.setFileLoading(midiFileStore.loading(), midiFileStore.progress());
                inputKeys.selectMidiPlayback();
            }
            else {
                JsonDocument log;
                log["schema"] = 1;
                log["app"] = AppName;
                log["version"] = AppVersion;
                log["uptime_us"] = nowUs;
                log["rx"] = app.state().stats.midi_events_rx;
                log["tx"] = app.state().stats.midi_events_tx;
                log["drops"] = app.state().stats.midi_events_dropped;
                log["late_max_us"] = app.scheduler().stats().max_late_us;
                log["late_average_us"] = app.scheduler().stats().late_count ? app.scheduler().stats().total_late_us / app.scheduler().stats().late_count : 0;
                log["din_drops"] = dinMidi.dropped();
                log["din_high_water"] = dinMidi.highWater();
                log["din_rx_bytes"] = dinInput.bytes();
                log["din_rx_events"] = dinInput.events();
                log["din_rx_errors"] = dinInput.errors();
                log["input_port"] = static_cast<unsigned>(inputSelection.active());
                log["ble_drops"] = bleMidi.diagnostics().events_dropped;
                log["ble_subscription_attempts"] = bleMidi.diagnostics().subscription_attempts;
                log["ble_subscription_status"] = bleMidi.diagnostics().subscription_status;
                log["ble_trace_dropped"] = bleMidi.diagnostics().trace_dropped;
                auto trace = log["ble_trace"].to<JsonArray>();
                for (std::size_t i = 0; i < bleMidi.trace().size(); ++i) {
                    const auto& entry = bleMidi.trace().at(i);
                    auto row = trace.add<JsonObject>();
                    row["at_us"] = entry.at_us;
                    row["event"] = bleTraceName(entry.event);
                    row["status"] = entry.status;
                    row["handle"] = entry.handle;
                    row["length"] = entry.length;
                    char data[33]{};
                    for (std::size_t j = 0; j < std::min<std::size_t>(entry.length, sizeof(entry.data)); ++j)
                        std::snprintf(data + j * 2, sizeof(data) - j * 2, "%02X", entry.data[j]);
                    row["data"] = data;
                }
                log["usb_errors"] = usbMidi.diagnostics().errors;
                log["profile"] = profileStore.name();
                log["usb_product"] = usbMidi.diagnostics().product;
                log["ble_product"] = bleMidi.diagnostics().product;
                log["loop_drops"] = app.looper().dropped();
                auto history = log["history"].to<JsonArray>();
                for (std::size_t i=0;i<diagnostics.size();++i) {
                    const auto& sample = diagnostics.at(i);
                    auto row = history.add<JsonObject>();
                    row["at_us"] = sample.at_us;
                    row["rx"] = sample.rx;
                    row["tx"] = sample.tx;
                    row["drops"] = sample.dropped;
                    row["late_us"] = sample.late_us;
                    row["ble_drops"] = sample.ble_dropped;
                    row["din_drops"] = sample.din_dropped;
                    row["active"] = sample.active;
                    row["din_queue"] = sample.queued;
                    row["loop"] = sample.loop_mode;
                }
                ui.showOverlay(writeJsonFile(SdPaths::Diagnostics,log) ? "DIAGNOSTICS SAVED" : "LOG SAVE FAILED",nowUs);
            }
            if (action != SemanticAction::DiagnosticsExport && action != SemanticAction::MidiFileBrowse && action != SemanticAction::MidiFileLoad)
                ui.showOverlay(sessionStore.error(),nowUs);
        }
    }
    const uint32_t fingerprint = settingsStore.fingerprint(app.state());
    if (fingerprint != pendingSettingsFingerprint) {
        pendingSettingsFingerprint = fingerprint;
        settingsSaveDueUs = nowUs + 1500000ULL;
    }
    if (fingerprint != savedSettingsFingerprint && settingsSaveDueUs != 0 && nowUs >= settingsSaveDueUs && app.idle() && dinMidi.queued() == 0) {
        if (settingsStore.save(app.state())) {
            savedSettingsFingerprint = fingerprint;
            settingsSaveDueUs = 0;
        } else {
            ui.showOverlay("SETTINGS SAVE FAILED",nowUs);
            settingsSaveDueUs = nowUs + 5000000ULL;
        }
    }
    if (ui.view() != pendingView) {
        pendingView = ui.view(); viewSaveDueUs = nowUs + 1500000ULL;
    }
    if (pendingView != savedView && viewSaveDueUs && nowUs >= viewSaveDueUs && app.idle() && dinMidi.queued() == 0 && !midiFileStore.loading()) {
        if (settingsStore.saveView(pendingView)) { savedView = pendingView; viewSaveDueUs = 0; }
        else { ui.showOverlay("VIEW SAVE FAILED", nowUs); viewSaveDueUs = nowUs + 5000000ULL; }
    }
    ui.update(nowUs, app, usbMidi, bleMidi, profileStore, inputKeys);
    vTaskDelay(1);
}

#endif
