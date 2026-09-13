#pragma once

#include "midi/BleMidiDecoder.h"
#include "transport/MidiSource.h"
#include "transport/BleMidiSubscription.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

#ifdef ARDUINO
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#endif

namespace midibrain {

enum class BleMidiState : uint8_t {
    Disabled,
    Scanning,
    Connecting,
    Subscribing,
    Connected,
    Waiting
};

struct BleMidiDiagnostics {
    BleMidiState state{BleMidiState::Disabled};
    uint32_t connect_count{0};
    uint32_t disconnect_count{0};
    uint32_t notifications_received{0};
    uint32_t events_received{0};
    uint32_t events_dropped{0};
    uint32_t errors{0};
    uint32_t subscription_attempts{0};
    int32_t subscription_status{0};
    char product[65]{};
};

class BleMidiSource final : public MidiSource {
public:
    using MidiCallback = void (*)(void*, const MidiEvent&);
    using ConnectionCallback = void (*)(void*, bool);

    BleMidiSource(MidiCallback midiCallback, ConnectionCallback connectionCallback, void* context);
    bool begin() override;
    void poll() override;
    bool connected() const override;
    bool scanning() const;
    const BleMidiDiagnostics& diagnostics() const;
    static const char* stateName(BleMidiState state);

private:
#ifdef ARDUINO
    struct Candidate {
        uint8_t address[6]{};
        char name[65]{};
        esp_ble_addr_type_t address_type{BLE_ADDR_TYPE_PUBLIC};
    };

    class AdvertisedCallbacks final : public BLEAdvertisedDeviceCallbacks {
    public:
        BleMidiSource* owner{nullptr};
        void onResult(BLEAdvertisedDevice advertisedDevice) override;
    };

    class ClientCallbacks final : public BLEClientCallbacks {
    public:
        BleMidiSource* owner{nullptr};
        void onConnect(BLEClient* client) override;
        void onDisconnect(BLEClient* client) override;
    };

    static void scanComplete(BLEScanResults results);
    static void gattEvent(esp_gattc_cb_event_t event, esp_gatt_if_t interface, esp_ble_gattc_cb_param_t* parameter);
    static void queueDecoded(void* context, const MidiEvent& event);
    void handleAdvertisement(BLEAdvertisedDevice& advertisedDevice);
    void startScan(uint64_t nowUs);
    void connectCandidate(const Candidate& candidate, uint64_t nowUs);
    void handleDisconnect(uint64_t nowUs);
    void handleNotification(const uint8_t* data, std::size_t size);
    void pollSubscription(uint64_t nowUs);

    static BleMidiSource* instance_;
    BLEScan* scan_{nullptr};
    BLEClient* client_{nullptr};
    BleMidiSubscription subscription_{};
    std::atomic<bool> session_active_{false};
    std::atomic<uint16_t> characteristic_handle_{0};
    std::atomic<uint16_t> descriptor_handle_{0};
    std::atomic<uint16_t> connection_id_{0};
    std::atomic<esp_gatt_if_t> interface_{ESP_GATT_IF_NONE};
    std::atomic<int32_t> registration_result_{-1};
    std::atomic<int32_t> subscription_result_{-1};
    std::atomic<bool> data_seen_{false};
    AdvertisedCallbacks advertised_callbacks_{};
    ClientCallbacks client_callbacks_{};
    QueueHandle_t candidate_queue_{nullptr};
    StaticQueue_t candidate_queue_storage_{};
    uint8_t candidate_queue_buffer_[sizeof(Candidate)]{};
    QueueHandle_t event_queue_{nullptr};
    StaticQueue_t event_queue_storage_{};
    uint8_t event_queue_buffer_[sizeof(MidiEvent) * 32]{};
    std::atomic<bool> scan_complete_{false};
    std::atomic<bool> disconnect_pending_{false};
    std::atomic<bool> overflow_pending_{false};
    std::atomic<uint32_t> notifications_received_{0};
    std::atomic<uint32_t> events_received_{0};
    std::atomic<uint32_t> events_dropped_{0};
    uint64_t next_scan_us_{0};
#endif
    MidiCallback midi_callback_{nullptr};
    ConnectionCallback connection_callback_{nullptr};
    void* context_{nullptr};
    BleMidiDecoder decoder_{};
    BleMidiDiagnostics diagnostics_{};
    bool connected_{false};
};

}
