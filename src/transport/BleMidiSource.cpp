#include "transport/BleMidiSource.h"
#include "common/AppInfo.h"
#include "transport/BleMidiDiscovery.h"

#ifdef ARDUINO
#include <BLEDevice.h>
#include <BLERemoteCharacteristic.h>
#include <BLERemoteDescriptor.h>
#include <BLERemoteService.h>
#include <esp_timer.h>

#include <cstring>
#include <algorithm>
#include <cstdio>
#include <string>
#endif

namespace midibrain {

namespace {

constexpr const char* serviceUuid = "03b80e5a-ede8-4b33-a751-6ce34ec4c700";
constexpr const char* characteristicUuid = "7772e5db-3868-4112-a1a9-f2669d106bf3";

}

#ifdef ARDUINO
BleMidiSource* BleMidiSource::instance_{nullptr};
#endif

BleMidiSource::BleMidiSource(MidiCallback midiCallback, ConnectionCallback connectionCallback, void* context)
    : midi_callback_(midiCallback), connection_callback_(connectionCallback), context_(context) {}

bool BleMidiSource::begin() {
#ifdef ARDUINO
    candidate_queue_ = xQueueCreateStatic(1, sizeof(Candidate), candidate_queue_buffer_, &candidate_queue_storage_);
    event_queue_ = xQueueCreateStatic(32, sizeof(MidiEvent), event_queue_buffer_, &event_queue_storage_);
    trace_queue_ = xQueueCreateStatic(32, sizeof(BleTraceRecord), trace_queue_buffer_, &trace_queue_storage_);
    if (candidate_queue_ == nullptr || event_queue_ == nullptr || trace_queue_ == nullptr) {
        ++diagnostics_.errors;
        return false;
    }
    instance_ = this;
    advertised_callbacks_.owner = this;
    client_callbacks_.owner = this;
    BLEDevice::init(AppName);
    BLEDevice::setCustomGattcHandler(gattEvent);
    BLEDevice::setCustomGapHandler(gapEvent);
    BLEDevice::setPower(ESP_PWR_LVL_P9);
    scan_ = BLEDevice::getScan();
    client_ = BLEDevice::createClient();
    if (scan_ == nullptr || client_ == nullptr) {
        ++diagnostics_.errors;
        return false;
    }
    client_->setClientCallbacks(&client_callbacks_);
    scan_->setAdvertisedDeviceCallbacks(&advertised_callbacks_);
    scan_->setActiveScan(true);
    scan_->setInterval(96);
    scan_->setWindow(48);
    startScan(esp_timer_get_time());
    return true;
#else
    return false;
#endif
}

void BleMidiSource::poll() { poll(true); }

void BleMidiSource::poll(bool allowConnect) {
#ifdef ARDUINO
    if (!candidate_queue_ || !event_queue_ || !trace_queue_ || !client_ || !scan_) return;
    const uint64_t nowUs = esp_timer_get_time();
    BleTraceRecord traceEntry;
    while (xQueueReceive(trace_queue_, &traceEntry, 0) == pdTRUE) trace_.push(traceEntry);
    diagnostics_.trace_dropped = trace_dropped_.load();
    if (reconnect_requested_ && allowConnect) {
        reconnect_requested_ = false;
        record(BleTraceEvent::Reconnect);
        if (client_->isConnected()) {
            session_active_.store(false);
            client_->disconnect();
            return;
        }
        scan_->stop();
        handleDisconnect(nowUs);
    }
    diagnostics_.notifications_received = notifications_received_.load(std::memory_order_relaxed);
    diagnostics_.events_received = events_received_.load(std::memory_order_relaxed);
    diagnostics_.events_dropped = events_dropped_.load(std::memory_order_relaxed);
    diagnostics_.midi_received = connected_ && data_seen_.load(std::memory_order_acquire);
    if (disconnect_pending_.exchange(false, std::memory_order_acq_rel)) handleDisconnect(nowUs);
    if (session_active_.load(std::memory_order_acquire)) pollSubscription(nowUs);
    if (overflow_pending_.exchange(false, std::memory_order_acq_rel)) {
        xQueueReset(event_queue_);
        if (connection_callback_ != nullptr) connection_callback_(context_, false);
        ++diagnostics_.errors;
    }
    MidiEvent event;
    while (xQueueReceive(event_queue_, &event, 0) == pdTRUE) {
        if (connected_ && session_active_.load(std::memory_order_acquire) && midi_callback_ != nullptr) {
            midi_callback_(context_, event);
        }
    }
    if (connected_ || diagnostics_.state == BleMidiState::Subscribing || diagnostics_.state == BleMidiState::Connecting) return;
    Candidate candidate;
    if (allowConnect && xQueueReceive(candidate_queue_, &candidate, 0) == pdTRUE) {
        connectCandidate(candidate, nowUs);
        return;
    }
    if (scan_complete_.exchange(false, std::memory_order_acq_rel)) {
        if (scan_ != nullptr) scan_->clearResults();
        diagnostics_.state = BleMidiState::Waiting;
        next_scan_us_ = nowUs + 1000000ULL;
    }
    if (diagnostics_.state == BleMidiState::Waiting && nowUs >= next_scan_us_) startScan(nowUs);
#else
    static_cast<void>(allowConnect);
#endif
}

bool BleMidiSource::connected() const { return connected_; }
bool BleMidiSource::scanning() const { return diagnostics_.state == BleMidiState::Scanning; }
const BleMidiDiagnostics& BleMidiSource::diagnostics() const { return diagnostics_; }

const char* BleMidiSource::stateName(BleMidiState state) {
    switch (state) {
        case BleMidiState::Disabled: return "OFF";
        case BleMidiState::Scanning: return "SCAN";
        case BleMidiState::Connecting: return "LINK";
        case BleMidiState::Subscribing: return "SUB";
        case BleMidiState::Connected: return "ON";
        case BleMidiState::Waiting: return "WAIT";
    }
    return "OFF";
}

#ifdef ARDUINO
void BleMidiSource::AdvertisedCallbacks::onResult(BLEAdvertisedDevice advertisedDevice) {
    if (owner != nullptr) owner->handleAdvertisement(advertisedDevice);
}

void BleMidiSource::ClientCallbacks::onConnect(BLEClient* client) {
    static_cast<void>(client);
}

void BleMidiSource::ClientCallbacks::onDisconnect(BLEClient* client) {
    static_cast<void>(client);
    if (owner != nullptr) {
        owner->session_active_.store(false, std::memory_order_release);
        owner->disconnect_pending_.store(true, std::memory_order_release);
    }
}

void BleMidiSource::scanComplete(BLEScanResults results) {
    static_cast<void>(results);
    if (instance_ != nullptr) instance_->scan_complete_.store(true, std::memory_order_release);
}

void BleMidiSource::gattEvent(esp_gattc_cb_event_t event, esp_gatt_if_t interface, esp_ble_gattc_cb_param_t* parameter) {
    auto* source = instance_;
    if (!source || !parameter) return;
    if (event == ESP_GATTC_CONNECT_EVT && source->client_ && interface == source->client_->getGattcIf()) {
        const uint16_t previousConnection = source->client_->getConnId();
        source->record(BleTraceEvent::Link, previousConnection, parameter->connect.conn_id);
        if (source->mtu_guard_.connected(previousConnection, parameter->connect.conn_id)) {
            const auto result = esp_ble_gattc_send_mtu_req(interface, parameter->connect.conn_id);
            source->record(BleTraceEvent::MtuRequest, result, parameter->connect.conn_id);
        }
    }
    if (event == ESP_GATTC_DISCONNECT_EVT) source->mtu_guard_.reset();
    if (event == ESP_GATTC_OPEN_EVT) source->record(BleTraceEvent::Open, parameter->open.status, parameter->open.conn_id);
    else if (event == ESP_GATTC_SEARCH_CMPL_EVT) source->record(BleTraceEvent::Discovery, parameter->search_cmpl.status, parameter->search_cmpl.conn_id);
    else if (event == ESP_GATTC_DISCONNECT_EVT) source->record(BleTraceEvent::Disconnect, parameter->disconnect.reason, parameter->disconnect.conn_id);
    else if (event == ESP_GATTC_CFG_MTU_EVT) source->record(BleTraceEvent::Mtu, parameter->cfg_mtu.status, parameter->cfg_mtu.mtu);
    if (event == ESP_GATTC_NOTIFY_EVT && (!source->session_active_.load(std::memory_order_acquire) ||
        interface != source->interface_.load() || parameter->notify.conn_id != source->connection_id_.load() ||
        parameter->notify.handle != source->characteristic_handle_.load() || !parameter->notify.is_notify)) {
        if (source->rejected_notifications_.fetch_add(1) < 16)
            source->record(BleTraceEvent::RejectedNotification, parameter->notify.conn_id, parameter->notify.handle,
                parameter->notify.value, parameter->notify.value_len);
        return;
    }
    if (!source->session_active_.load(std::memory_order_acquire) ||
        interface != source->interface_.load(std::memory_order_relaxed)) return;
    const uint16_t connection = source->connection_id_.load(std::memory_order_relaxed);
    const uint16_t characteristic = source->characteristic_handle_.load(std::memory_order_relaxed);
    if (event == ESP_GATTC_READ_CHAR_EVT && parameter->read.conn_id == connection && parameter->read.handle == characteristic) {
        source->record(BleTraceEvent::ReadResult, parameter->read.status, characteristic, parameter->read.value, parameter->read.value_len);
        source->read_result_.store(parameter->read.status, std::memory_order_release);
    } else if (event == ESP_GATTC_REG_FOR_NOTIFY_EVT && parameter->reg_for_notify.handle == characteristic) {
        source->record(BleTraceEvent::RegisterResult, parameter->reg_for_notify.status, characteristic);
        source->registration_result_.store(parameter->reg_for_notify.status, std::memory_order_release);
    } else if (event == ESP_GATTC_WRITE_DESCR_EVT && parameter->write.conn_id == connection &&
               parameter->write.handle == source->descriptor_handle_.load(std::memory_order_relaxed)) {
        source->record(BleTraceEvent::WriteResult, parameter->write.status, parameter->write.handle);
        source->subscription_result_.store(parameter->write.status, std::memory_order_release);
    } else if (event == ESP_GATTC_NOTIFY_EVT && parameter->notify.conn_id == connection &&
               parameter->notify.handle == characteristic && parameter->notify.is_notify) {
        source->handleNotification(parameter->notify.value, parameter->notify.value_len);
    }
}

void BleMidiSource::gapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* parameter) {
    if (!instance_ || !parameter) return;
    if (event == ESP_GAP_BLE_SEC_REQ_EVT) instance_->record(BleTraceEvent::SecurityRequest);
    else if (event == ESP_GAP_BLE_AUTH_CMPL_EVT) {
        instance_->record(BleTraceEvent::Authentication, parameter->ble_security.auth_cmpl.success ? 0 : parameter->ble_security.auth_cmpl.fail_reason);
        instance_->authentication_result_.store(parameter->ble_security.auth_cmpl.success ? 0 : 1, std::memory_order_release);
    }
    else if (event == ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT)
        instance_->record(BleTraceEvent::ConnectionParameters, parameter->update_conn_params.status, parameter->update_conn_params.conn_int);
}

void BleMidiSource::queueDecoded(void* context, const MidiEvent& event) {
    auto* source = static_cast<BleMidiSource*>(context);
    if (xQueueSend(source->event_queue_, &event, 0) == pdTRUE) {
        source->events_received_.fetch_add(1, std::memory_order_relaxed);
    } else {
        source->events_dropped_.fetch_add(1, std::memory_order_relaxed);
        source->overflow_pending_.store(true, std::memory_order_release);
    }
}

void BleMidiSource::handleAdvertisement(BLEAdvertisedDevice& advertisedDevice) {
    const BLEUUID midiService(serviceUuid);
    const bool serviceMatch = advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(midiService);
    const std::string name = advertisedDevice.haveName() ? advertisedDevice.getName() : std::string{};
    if (!bleMidiCandidate(serviceMatch, name.c_str())) return;
    Candidate candidate;
    BLEAddress address = advertisedDevice.getAddress();
    std::memcpy(candidate.address, *address.getNative(), sizeof(candidate.address));
    candidate.address_type = advertisedDevice.getAddressType();
    std::snprintf(candidate.name,sizeof(candidate.name),"%s",name.c_str());
    if (xQueueSend(candidate_queue_, &candidate, 0) != pdTRUE) return;
    record(BleTraceEvent::Candidate, advertisedDevice.getRSSI(), serviceMatch ? 1 : 0);
    if (scan_ != nullptr) scan_->stop();
}

void BleMidiSource::startScan(uint64_t nowUs) {
    static_cast<void>(nowUs);
    if (scan_ == nullptr) return;
    record(BleTraceEvent::Scan);
    scan_complete_.store(false, std::memory_order_release);
    diagnostics_.state = BleMidiState::Scanning;
    if (!scan_->start(5, scanComplete, false)) {
        record(BleTraceEvent::Error, -1);
        ++diagnostics_.errors;
        diagnostics_.state = BleMidiState::Waiting;
        next_scan_us_ = esp_timer_get_time() + 1000000ULL;
    }
}

void BleMidiSource::connectCandidate(const Candidate& candidate, uint64_t nowUs) {
    static_cast<void>(nowUs);
    diagnostics_.state = BleMidiState::Connecting;
    scan_complete_.store(false, std::memory_order_release);
    xQueueReset(candidate_queue_);
    BLEAddress address(const_cast<uint8_t*>(candidate.address));
    record(BleTraceEvent::Connect, candidate.address_type);
    if (!client_->connect(address, candidate.address_type)) {
        record(BleTraceEvent::Error, -2);
        ++diagnostics_.errors;
        diagnostics_.state = BleMidiState::Waiting;
        next_scan_us_ = esp_timer_get_time() + 1000000ULL;
        return;
    }
    client_->getServices();
    BLERemoteService* service = client_->getService(BLEUUID(serviceUuid));
    if (service == nullptr) {
        record(BleTraceEvent::Error, -3);
        ++diagnostics_.errors;
        client_->disconnect();
        return;
    }
    BLERemoteCharacteristic* characteristic = service->getCharacteristic(BLEUUID(characteristicUuid));
    if (characteristic == nullptr || !characteristic->canNotify()) {
        record(BleTraceEvent::Error, -4);
        ++diagnostics_.errors;
        client_->disconnect();
        return;
    }
    BLERemoteDescriptor* descriptor = characteristic->getDescriptor(BLEUUID(static_cast<uint16_t>(0x2902)));
    if (descriptor == nullptr || !client_->isConnected() || disconnect_pending_.load(std::memory_order_acquire)) {
        record(BleTraceEvent::Error, -5);
        ++diagnostics_.errors;
        client_->disconnect();
        return;
    }
    characteristic_handle_.store(characteristic->getHandle(), std::memory_order_relaxed);
    descriptor_handle_.store(descriptor->getHandle(), std::memory_order_relaxed);
    connection_id_.store(client_->getConnId(), std::memory_order_relaxed);
    interface_.store(client_->getGattcIf(), std::memory_order_relaxed);
    registration_result_.store(-1, std::memory_order_relaxed);
    subscription_result_.store(-1, std::memory_order_relaxed);
    read_result_.store(-1, std::memory_order_relaxed);
    authentication_result_.store(-1, std::memory_order_relaxed);
    rejected_notifications_.store(0);
    data_seen_.store(false, std::memory_order_relaxed);
    session_notifications_.store(0);
    record(BleTraceEvent::Handles, descriptor->getHandle(), characteristic->getHandle());
    decoder_.reset();
    std::snprintf(diagnostics_.product,sizeof(diagnostics_.product),"%s",candidate.name);
    subscription_.start(esp_timer_get_time());
    diagnostics_.subscription_status = ESP_GATT_OK;
    diagnostics_.state = BleMidiState::Subscribing;
    session_active_.store(true, std::memory_order_release);
}

void BleMidiSource::pollSubscription(uint64_t nowUs) {
    const int32_t read = read_result_.exchange(-1, std::memory_order_acq_rel);
    if (read >= 0) {
        diagnostics_.subscription_status = read;
        subscription_.readComplete(read == ESP_GATT_OK, nowUs, read == ESP_GATT_INSUF_AUTHENTICATION || read == ESP_GATT_INSUF_ENCRYPTION);
        if (read != ESP_GATT_OK) ++diagnostics_.errors;
    }
    const int32_t authenticated = authentication_result_.exchange(-1, std::memory_order_acq_rel);
    if (authenticated >= 0) subscription_.encrypted(authenticated == 0, nowUs);
    const int32_t registered = registration_result_.exchange(-1, std::memory_order_acq_rel);
    if (registered >= 0) {
        diagnostics_.subscription_status = registered;
        subscription_.registered(registered == ESP_GATT_OK, nowUs);
        if (registered != ESP_GATT_OK) ++diagnostics_.errors;
    }
    const int32_t subscribed = subscription_result_.exchange(-1, std::memory_order_acq_rel);
    if (subscribed >= 0) {
        diagnostics_.subscription_status = subscribed;
        subscription_.subscribed(subscribed == ESP_GATT_OK, nowUs);
        if (subscribed != ESP_GATT_OK) ++diagnostics_.errors;
        if (subscribed == ESP_GATT_INSUF_AUTHENTICATION || subscribed == ESP_GATT_INSUF_ENCRYPTION) {
            BLEAddress address = client_->getPeerAddress();
            const auto result = esp_ble_set_encryption(*address.getNative(), ESP_BLE_SEC_ENCRYPT_NO_MITM);
            record(BleTraceEvent::Error, result, descriptor_handle_.load());
        }
    }
    const auto operation = subscription_.poll(nowUs);
    esp_err_t result = ESP_OK;
    if (operation == BleMidiSubscription::Operation::Encrypt) {
        BLEAddress address = client_->getPeerAddress();
        result = esp_ble_set_encryption(*address.getNative(), ESP_BLE_SEC_ENCRYPT_NO_MITM);
        if (result != ESP_OK) subscription_.encrypted(false, nowUs);
    } else if (operation == BleMidiSubscription::Operation::Read) {
        record(BleTraceEvent::Read, 0, characteristic_handle_.load());
        result = esp_ble_gattc_read_char(interface_.load(), connection_id_.load(), characteristic_handle_.load(), ESP_GATT_AUTH_REQ_NONE);
        if (result != ESP_OK) subscription_.readComplete(false, nowUs);
    } else if (operation == BleMidiSubscription::Operation::Register) {
        record(BleTraceEvent::Register, 0, characteristic_handle_.load());
        BLEAddress address = client_->getPeerAddress();
        result = esp_ble_gattc_register_for_notify(interface_.load(), *address.getNative(), characteristic_handle_.load());
        if (result != ESP_OK) subscription_.registered(false, nowUs);
    } else if (operation == BleMidiSubscription::Operation::Enable) {
        uint8_t value[]{1, 0};
        record(BleTraceEvent::Enable, 0, descriptor_handle_.load());
        ++diagnostics_.subscription_attempts;
        result = esp_ble_gattc_write_char_descr(interface_.load(), connection_id_.load(), descriptor_handle_.load(),
                                               sizeof(value), value, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
        if (result != ESP_OK) subscription_.subscribed(false, nowUs);
    } else if (operation == BleMidiSubscription::Operation::Disconnect) {
        ++diagnostics_.errors;
        if (diagnostics_.subscription_status == ESP_GATT_OK) diagnostics_.subscription_status = ESP_ERR_TIMEOUT;
        session_active_.store(false, std::memory_order_release);
        xQueueReset(event_queue_);
        if (connected_) {
            connected_ = false;
            ++diagnostics_.disconnect_count;
            if (connection_callback_ != nullptr) connection_callback_(context_, false);
        }
        diagnostics_.state = BleMidiState::Connecting;
        client_->disconnect();
        return;
    }
    if (result != ESP_OK) {
        record(BleTraceEvent::Error, result);
        ++diagnostics_.errors;
        diagnostics_.subscription_status = result;
    }
    if (!connected_ && subscription_.ready()) {
        record(BleTraceEvent::Ready);
        connected_ = true;
        diagnostics_.state = BleMidiState::Connected;
        ++diagnostics_.connect_count;
        if (connection_callback_ != nullptr) connection_callback_(context_, true);
    }
}

void BleMidiSource::handleDisconnect(uint64_t nowUs) {
    static_cast<void>(nowUs);
    const bool wasConnected = connected_;
    connected_ = false;
    session_active_.store(false, std::memory_order_release);
    subscription_.reset();
    xQueueReset(event_queue_);
    xQueueReset(candidate_queue_);
    scan_complete_.store(false, std::memory_order_release);
    diagnostics_.state = BleMidiState::Waiting;
    next_scan_us_ = esp_timer_get_time() + 500000ULL;
    if (wasConnected) {
        ++diagnostics_.disconnect_count;
        if (connection_callback_ != nullptr) connection_callback_(context_, false);
    }
}

void BleMidiSource::handleNotification(const uint8_t* data, std::size_t size) {
    notifications_received_.fetch_add(1, std::memory_order_relaxed);
    const auto decoded = decoder_.decode(data, size, esp_timer_get_time(), queueDecoded, this);
    if (decoded) data_seen_.store(true, std::memory_order_release);
    if (session_notifications_.fetch_add(1) < 16)
        record(BleTraceEvent::Notification, decoded, characteristic_handle_.load(), data, size);
}

void BleMidiSource::record(BleTraceEvent event, int32_t status, uint16_t handle, const uint8_t* data, uint16_t size) {
    BleTraceRecord entry;
    entry.at_us = esp_timer_get_time();
    entry.event = event;
    entry.status = status;
    entry.handle = handle;
    entry.length = size;
    if (data) std::memcpy(entry.data, data, std::min<std::size_t>(size, sizeof(entry.data)));
    if (!trace_queue_ || xQueueSend(trace_queue_, &entry, 0) != pdTRUE) trace_dropped_.fetch_add(1);
}
#endif

}
