#include "transport/UsbMidiSource.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <esp_intr_alloc.h>
#endif

namespace midibrain {

UsbMidiSource::UsbMidiSource(MidiCallback midiCallback, ConnectionCallback connectionCallback, void* context)
    : midi_callback_(midiCallback), connection_callback_(connectionCallback), context_(context) {}

bool UsbMidiSource::begin() {
#ifdef ARDUINO
    usb_host_config_t hostConfiguration{};
    hostConfiguration.skip_phy_setup = false;
    hostConfiguration.intr_flags = ESP_INTR_FLAG_LEVEL1;
    if (usb_host_install(&hostConfiguration) != ESP_OK) {
        ++diagnostics_.errors;
        return false;
    }
    usb_host_client_config_t clientConfiguration{};
    clientConfiguration.is_synchronous = false;
    clientConfiguration.max_num_event_msg = 8;
    clientConfiguration.async.client_event_callback = clientCallback;
    clientConfiguration.async.callback_arg = this;
    if (usb_host_client_register(&clientConfiguration, &client_) != ESP_OK) {
        ++diagnostics_.errors;
        usb_host_uninstall();
        return false;
    }
    return true;
#else
    return false;
#endif
}

void UsbMidiSource::poll() {
#ifdef ARDUINO
    if (client_ == nullptr) {
        return;
    }
    usb_host_client_handle_events(client_, 0);
    uint32_t flags = 0;
    usb_host_lib_handle_events(0, &flags);
#endif
}

bool UsbMidiSource::connected() const { return connected_; }
const UsbDiagnostics& UsbMidiSource::diagnostics() const { return diagnostics_; }

#ifdef ARDUINO
void UsbMidiSource::clientCallback(const usb_host_client_event_msg_t* message, void* argument) {
    if (message != nullptr && argument != nullptr) {
        static_cast<UsbMidiSource*>(argument)->handleClientEvent(*message);
    }
}

void UsbMidiSource::transferCallback(usb_transfer_t* transfer) {
    if (transfer != nullptr && transfer->context != nullptr) {
        static_cast<UsbMidiSource*>(transfer->context)->handleTransfer(*transfer);
    }
}

void UsbMidiSource::handleClientEvent(const usb_host_client_event_msg_t& message) {
    if (message.event == USB_HOST_CLIENT_EVENT_NEW_DEV && device_ == nullptr) {
        openDevice(message.new_dev.address);
    } else if (message.event == USB_HOST_CLIENT_EVENT_DEV_GONE && message.dev_gone.dev_hdl == device_) {
        closeDevice();
    }
}

void UsbMidiSource::openDevice(uint8_t address) {
    if (usb_host_device_open(client_, address, &device_) != ESP_OK) {
        ++diagnostics_.errors;
        return;
    }
    const usb_device_desc_t* deviceDescriptor = nullptr;
    if (usb_host_get_device_descriptor(device_, &deviceDescriptor) == ESP_OK && deviceDescriptor != nullptr) {
        diagnostics_.vid = deviceDescriptor->idVendor;
        diagnostics_.pid = deviceDescriptor->idProduct;
    }
    const usb_config_desc_t* configuration = nullptr;
    usb_device_info_t info{};
    if (usb_host_device_info(device_, &info) == ESP_OK) {
        const auto copyString = [](const usb_str_desc_t* descriptor, char* output) {
            std::size_t count = 0;
            if (descriptor && descriptor->bLength >= 2) {
                for (; count < 64 && count < (descriptor->bLength - 2U) / 2U; ++count) {
                    const uint16_t ch = descriptor->wData[count];
                    output[count] = ch >= 32 && ch < 127 ? static_cast<char>(ch) : '?';
                }
            }
            output[count] = '\0';
        };
        copyString(info.str_desc_manufacturer,diagnostics_.manufacturer);
        copyString(info.str_desc_product,diagnostics_.product);
    }
    if (usb_host_get_active_config_descriptor(device_, &configuration) != ESP_OK || configuration == nullptr) {
        ++diagnostics_.errors;
        closeDevice();
        return;
    }
    parseConfiguration(configuration);
    if (!diagnostics_.midi_interface_found) {
        ++diagnostics_.errors;
    }
}

void UsbMidiSource::parseConfiguration(const usb_config_desc_t* configuration) {
    const uint8_t* cursor = configuration->val;
    const uint8_t* end = cursor + configuration->wTotalLength;
    bool insideMidiInterface = false;
    while (cursor + 2 <= end && cursor[0] >= 2 && cursor + cursor[0] <= end) {
        const uint8_t type = cursor[1];
        if (type == USB_B_DESCRIPTOR_TYPE_INTERFACE) {
            if (cursor[0] < sizeof(usb_intf_desc_t)) { ++diagnostics_.errors; break; }
            const auto* interfaceDescriptor = reinterpret_cast<const usb_intf_desc_t*>(cursor);
            insideMidiInterface = interfaceDescriptor->bInterfaceClass == USB_CLASS_AUDIO && interfaceDescriptor->bInterfaceSubClass == 3;
            interface_number_ = interfaceDescriptor->bInterfaceNumber;
            alternate_setting_ = interfaceDescriptor->bAlternateSetting;
        } else if (type == USB_B_DESCRIPTOR_TYPE_ENDPOINT && insideMidiInterface && input_transfer_ == nullptr) {
            if (cursor[0] < sizeof(usb_ep_desc_t)) { ++diagnostics_.errors; break; }
            const auto* endpoint = reinterpret_cast<const usb_ep_desc_t*>(cursor);
            const bool input = (endpoint->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) != 0;
            const uint8_t transferType = endpoint->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK;
            if (input && (transferType == USB_BM_ATTRIBUTES_XFER_BULK || transferType == USB_BM_ATTRIBUTES_XFER_INT)) {
                if (!endpoint->wMaxPacketSize || endpoint->wMaxPacketSize > 1024
                    || usb_host_interface_claim(client_,device_,interface_number_,alternate_setting_) != ESP_OK) {
                    ++diagnostics_.errors;
                    cursor += cursor[0];
                    continue;
                }
                diagnostics_.interface_number = interface_number_;
                diagnostics_.midi_interface_found = true;
                if (usb_host_transfer_alloc(endpoint->wMaxPacketSize, 0, &input_transfer_) == ESP_OK) {
                    diagnostics_.endpoint_address = endpoint->bEndpointAddress;
                    input_transfer_->device_handle = device_;
                    input_transfer_->bEndpointAddress = endpoint->bEndpointAddress;
                    input_transfer_->callback = transferCallback;
                    input_transfer_->context = this;
                    input_transfer_->num_bytes = endpoint->wMaxPacketSize;
                    if (usb_host_transfer_submit(input_transfer_) == ESP_OK) {
                        connected_ = true;
                        ++diagnostics_.connect_count;
                        if (connection_callback_ != nullptr) connection_callback_(context_, true);
                    } else {
                        usb_host_transfer_free(input_transfer_);
                        input_transfer_ = nullptr;
                        ++diagnostics_.errors;
                    }
                }
                if (connected_) return;
                usb_host_interface_release(client_,device_,interface_number_);
                diagnostics_.midi_interface_found = false;
            }
        }
        cursor += cursor[0];
    }
}

void UsbMidiSource::handleTransfer(usb_transfer_t& transfer) {
    if (transfer.status == USB_TRANSFER_STATUS_COMPLETED) {
        ++diagnostics_.packets_received;
        for (int offset = 0; offset + 3 < transfer.actual_num_bytes; offset += 4) {
            const uint8_t* packet = transfer.data_buffer + offset;
            if ((packet[0] & 0x0fU) != 0 && midi_callback_ != nullptr) {
                midi_callback_(context_, parser_.parseUsbPacket(packet, esp_timer_get_time()));
            }
        }
        if (connected_ && usb_host_transfer_submit(&transfer) != ESP_OK) {
            ++diagnostics_.errors;
        }
    } else if (transfer.status != USB_TRANSFER_STATUS_CANCELED && transfer.status != USB_TRANSFER_STATUS_NO_DEVICE) {
        ++diagnostics_.errors;
        if (connected_) usb_host_transfer_submit(&transfer);
    }
}

void UsbMidiSource::closeDevice() {
    const bool wasConnected = connected_;
    connected_ = false;
    if (input_transfer_ != nullptr) {
        usb_host_transfer_free(input_transfer_);
        input_transfer_ = nullptr;
    }
    if (device_ != nullptr && diagnostics_.midi_interface_found) {
        usb_host_interface_release(client_, device_, interface_number_);
    }
    if (device_ != nullptr) {
        usb_host_device_close(client_, device_);
        device_ = nullptr;
    }
    diagnostics_.midi_interface_found = false;
    diagnostics_.endpoint_address = 0;
    if (wasConnected) {
        ++diagnostics_.disconnect_count;
        if (connection_callback_ != nullptr) connection_callback_(context_, false);
    }
}
#endif

}
