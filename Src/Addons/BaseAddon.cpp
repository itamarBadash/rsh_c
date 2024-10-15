#include "BaseAddon.h"
#include <iostream>

BaseAddon::BaseAddon(const std::string& new_name, libusb_device_handle* dev_handle, uint8_t bus, uint8_t address)
    : name(new_name), device_handle(dev_handle), bus_number(bus), device_address(address) {
}

BaseAddon::~BaseAddon() {
    if (device_handle) {
        libusb_close(device_handle);  // Close the device handle when the addon is destroyed
    }
}

uint8_t BaseAddon::getBusNumber() const {
    return bus_number;
}

uint8_t BaseAddon::getDeviceAddress() const {
    return device_address;
}

BaseAddon::Result BaseAddon::Activate() {
    if (device_handle) {
        std::cout << "Activating " << name << " on bus " << (int)bus_number << " address " << (int)device_address << std::endl;

        // Get and print device descriptor information
        libusb_device_descriptor desc;
        int result = libusb_get_device_descriptor(libusb_get_device(device_handle), &desc);
        if (result == 0) {
            std::cout << "Device Class: " << (int)desc.bDeviceClass << std::endl;
            std::cout << "Vendor ID: " << desc.idVendor << std::endl;
            std::cout << "Product ID: " << desc.idProduct << std::endl;
        } else {
            std::cerr << "Failed to get device descriptor: " << libusb_error_name(result) << std::endl;
            return Result::Failure;
        }

        // Prepare buffer to hold status
        uint8_t buffer[2];  // Buffer to hold the status

        // Send a standard GET_STATUS request
        result = libusb_control_transfer(device_handle,
                                         LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
                                         LIBUSB_REQUEST_GET_STATUS,  // Standard GET_STATUS request
                                         0,  // wValue (not used for GET_STATUS)
                                         0,  // wIndex (0 for device-wide status)
                                         buffer,  // Buffer to store status
                                         sizeof(buffer),  // Size of the buffer (2 bytes)
                                         1000);  // Timeout in milliseconds

        if (result < 0) {
            std::cerr << "Failed to send GET_STATUS: " << libusb_error_name(result) << std::endl;
            return Result::Failure;
        }

        // Parse the status
        uint16_t status = buffer[0] | (buffer[1] << 8);
        std::cout << "Device status: " << status << std::endl;

        return Result::Success;
    }
    return Result::ConnectionError;
}

BaseAddon::Result BaseAddon::Deactivate() {
    if (device_handle) {
        std::cout << "Deactivating " << name << " on bus " << (int)bus_number << " address " << (int)device_address << std::endl;

        // Detach kernel driver (if applicable)
        int result = libusb_detach_kernel_driver(device_handle, 0);  // Detach interface 0
        if (result == 0) {
            std::cout << "Kernel driver detached successfully." << std::endl;
        } else if (result != LIBUSB_ERROR_NOT_FOUND) {
            std::cerr << "Failed to detach kernel driver: " << libusb_error_name(result) << std::endl;
            return Result::Failure;
        }

        // Claim the interface after detaching kernel driver
        result = libusb_claim_interface(device_handle, 0);  // Assuming interface 0
        if (result < 0) {
            std::cerr << "Failed to claim interface: " << libusb_error_name(result) << std::endl;
            return Result::Failure;
        }

        // Clear halt in case the endpoint is stalled
        libusb_clear_halt(device_handle, LIBUSB_ENDPOINT_OUT);

        // Send a control transfer to deactivate the device
        result = libusb_control_transfer(device_handle,
                                         LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
                                         0x01,      // Deactivation command (vendor-specific)
                                         0x0000,    // Deactivation value (0 for deactivate)
                                         0x0000,    // Index (usually 0 for device-wide command)
                                         nullptr,   // No data payload
                                         0,         // Data length
                                         1000);     // Timeout in milliseconds

        if (result < 0) {
            std::cerr << "Failed to deactivate device: " << libusb_error_name(result) << std::endl;
            return Result::Failure;
        }

        std::cout << name << " deactivated successfully." << std::endl;
        return Result::Success;
    }
    return Result::ConnectionError;
}
