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
        return SendCommand(0x01, 1, 0);  // Example command to activate the device
    }
    std::cout << name << "(no USB device connected)." << std::endl;
    return Result::ConnectionError;
}

BaseAddon::Result BaseAddon::Deactivate() {
    if (device_handle) {
        return SendCommand(0x01, 0, 0);  // Example command to deactivate the device
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
