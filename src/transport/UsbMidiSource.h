#pragma once

#include "midi/MidiEvent.h"
#include "midi/MidiParser.h"
#include "transport/MidiSource.h"

#include <cstdint>

#ifdef ARDUINO
#include <usb/usb_host.h>
#endif

namespace midibrain {

struct UsbDiagnostics {
    uint16_t vid{0};
    uint16_t pid{0};
    char manufacturer[65]{};
    char product[65]{};
    uint8_t interface_number{0};
    uint8_t endpoint_address{0};
    uint32_t connect_count{0};
    uint32_t disconnect_count{0};
    uint32_t packets_received{0};
    uint32_t errors{0};
    bool midi_interface_found{false};
};

class UsbMidiSource final : public MidiSource {
public:
    using MidiCallback = void (*)(void*, const MidiEvent&);
    using ConnectionCallback = void (*)(void*, bool);

    UsbMidiSource(MidiCallback midiCallback, ConnectionCallback connectionCallback, void* context);
    bool begin() override;
    void poll() override;
    bool connected() const override;
    const UsbDiagnostics& diagnostics() const;

private:
#ifdef ARDUINO
    static void clientCallback(const usb_host_client_event_msg_t* message, void* argument);
    static void transferCallback(usb_transfer_t* transfer);
    void handleClientEvent(const usb_host_client_event_msg_t& message);
    void handleTransfer(usb_transfer_t& transfer);
    void openDevice(uint8_t address);
    void parseConfiguration(const usb_config_desc_t* configuration);
    void closeDevice();

    usb_host_client_handle_t client_{nullptr};
    usb_device_handle_t device_{nullptr};
    usb_transfer_t* input_transfer_{nullptr};
    uint8_t interface_number_{0};
    uint8_t alternate_setting_{0};
#endif
    MidiCallback midi_callback_{nullptr};
    ConnectionCallback connection_callback_{nullptr};
    void* context_{nullptr};
    MidiParser parser_{};
    UsbDiagnostics diagnostics_{};
    bool connected_{false};
};

}
