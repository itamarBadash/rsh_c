#include "BaseAddon.h"
#include <iostream>

BaseAddon::BaseAddon(const std::string& new_name, libusb_device_handle* dev_handle)
    : name(new_name), device_handle(dev_handle) {
}

BaseAddon::~BaseAddon() {
    if (device_handle) {
        libusb_close(device_handle);  // Close the USB device handle if it's open
    }
}

std::string BaseAddon::getName() const {
    return name;
}

BaseAddon::Result BaseAddon::Activate() {
    if (device_handle) {
        // Ensure the correct interface is claimed before sending commands
        int claim_result = libusb_claim_interface(device_handle, 0);
        if (claim_result < 0) {
            std::cerr << "Failed to claim interface: " << libusb_error_name(claim_result) << std::endl;
            return Result::ConnectionError;
        }

        // Send the activation command (0x01, value = 1)
        return SendCommand(0x01, 1, 0);
    }

    std::cout << name << " (no USB device connected)." << std::endl;
    return Result::ConnectionError;
}

BaseAddon::Result BaseAddon::Deactivate() {
    if (device_handle) {
        // Ensure the correct interface is claimed before sending commands
        int claim_result = libusb_claim_interface(device_handle, 0);
        if (claim_result < 0) {
            std::cerr << "Failed to claim interface: " << libusb_error_name(claim_result) << std::endl;
            return Result::ConnectionError;
        }

        // Send the deactivation command (0x01, value = 0)
        return SendCommand(0x01, 0, 0);
    }

    std::cout << name << " (no USB device connected)." << std::endl;
    return Result::ConnectionError;
}

BaseAddon::Result BaseAddon::GetStatus() {
    if (device_handle) {
        // Send a command to get the status of the USB device
        return SendCommand(0x02, 0, 0);  // Example command to get the status
    }
    std::cout << name << " (no USB device connected)." << std::endl;
    return Result::ConnectionError;
}

BaseAddon::Result BaseAddon::SendCommand(uint8_t request, uint16_t value, uint16_t index) {
    if (!device_handle) {
        std::cerr << "No USB device handle available." << std::endl;
        return Result::ConnectionError;
    }

    int result = libusb_control_transfer(device_handle,
                                         LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
                                         request,
                                         value,
                                         index,
                                         nullptr,  // No data payload for this example
                                         0,  // No data length
                                         1000);  // Timeout in milliseconds

    if (result < 0) {
        std::cerr << "Error sending control transfer: " << libusb_error_name(result) << std::endl;
        return Result::Failure;
    }

    std::cout << "Command sent successfully to " << name << std::endl;
    return Result::Success;
}
