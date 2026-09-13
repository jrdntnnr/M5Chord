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
#include "storage/AtomicFile.h"
#include "transport/BleMidiSource.h"
#include "transport/DinMidiSink.h"
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
Ui ui;
DiagnosticRing diagnostics;
uint64_t nextDiagnosticUs{0};
bool hardwareReady{false};
char lastInputStatus[96]{};
SemanticAction pendingStorage{SemanticAction::None};
AppState pendingPreset{};
uint8_t pendingSlot{0};
bool profileResolvePending{false};
uint32_t savedSettingsFingerprint{0};
uint32_t pendingSettingsFingerprint{0};
uint64_t settingsSaveDueUs{0};

void handleAction(void*, const ActionEvent& action, uint64_t nowUs);
InputKeys inputKeys(app, controllerMapper, profileStore, handleAction, nullptr);

void handleAction(void*, const ActionEvent& action, uint64_t nowUs) {
    if (inputKeys.handleAction(action, nowUs)) return;
    if (action.action == SemanticAction::ViewNext) {
        if (action.pressed) ui.nextView();
    } else if (action.action == SemanticAction::ViewPrev) {
        if (action.pressed) { ui.nextView(); ui.nextView(); ui.nextView(); }
    } else if (action.action == SemanticAction::PresetSave || action.action == SemanticAction::PresetLoad
        || action.action == SemanticAction::LoopSave || action.action == SemanticAction::LoopLoad
        || action.action == SemanticAction::DiagnosticsExport) {
        if (action.pressed) {
            if (pendingStorage != SemanticAction::None) { ui.showOverlay("STORAGE REQUEST PENDING",nowUs); return; }
            if (action.action == SemanticAction::PresetLoad || action.action == SemanticAction::LoopLoad) app.panic(nowUs);
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

void handleMidi(void*, const MidiEvent& event) {
    if (inputKeys.captureLearnEvent(event) || app.captureKeySelection(event)) { ++app.state().stats.midi_events_rx; return; }
    MappingResult mapped = controllerMapper.map(event);
    for (const auto& action : mapped.actions) handleAction(nullptr, action, event.timestamp_us);
    if (!mapped.consumed) app.receive(event);
    else ++app.state().stats.midi_events_rx;
}

void handleConnection(void*, bool connected) {
    if (!connected) { app.onUsbDisconnected(esp_timer_get_time()); controllerMapper.resetEdges(); }
    else profileResolvePending = true;
}

UsbMidiSource usbMidi(handleMidi, handleConnection, nullptr);
BleMidiSource bleMidi(handleMidi, handleConnection, nullptr);

}

void setup() {
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
    ui.showBootStatus("SETTINGS");
    settingsStore.load(app.state());
    savedSettingsFingerprint = settingsStore.fingerprint(app.state());
    pendingSettingsFingerprint = savedSettingsFingerprint;
    ui.showBootStatus("SD CARD");
    profileStore.begin();
    ui.showBootStatus("CONTROLLER PROFILE");
    profileStore.loadActive(controllerMapper);
    profileStore.applyInput(app.state());
    ui.showBootStatus("USB HOST");
    if (!usbMidi.begin()) {
        ui.showOverlay("USB INIT FAILED", esp_timer_get_time());
    }
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
    usbMidi.poll();
    bleMidi.poll();
    nowUs = esp_timer_get_time();
    app.tick(nowUs);
    if (dinMidi.takeRecovery()) app.panic(nowUs);
    dinMidi.poll();
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
            profileStore.resolve(usbMidi.connected() ? usb.vid : 0, usbMidi.connected() ? usb.pid : 0,
                usbMidi.connected() ? usb.manufacturer : "", usbMidi.connected() ? usb.product : bleMidi.diagnostics().product, controllerMapper);
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
                log["ble_drops"] = bleMidi.diagnostics().events_dropped;
                log["ble_subscription_attempts"] = bleMidi.diagnostics().subscription_attempts;
                log["ble_subscription_status"] = bleMidi.diagnostics().subscription_status;
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
                ui.showOverlay(writeJsonFile("/midi-brain/logs/diagnostics.json",log) ? "DIAGNOSTICS SAVED" : "LOG SAVE FAILED",nowUs);
            }
            if (action != SemanticAction::DiagnosticsExport) ui.showOverlay(sessionStore.error(),nowUs);
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
    ui.update(nowUs, app, usbMidi, bleMidi, profileStore, inputKeys);
    vTaskDelay(1);
}

#endif
