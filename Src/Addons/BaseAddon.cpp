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
        // Implement activation logic here...
        return Result::Success;
    }
    return Result::ConnectionError;
}

BaseAddon::Result BaseAddon::Deactivate() {
    if (device_handle) {
        std::cout << "Deactivating " << name << " on bus " << (int)bus_number << " address " << (int)device_address << std::endl;
        // Implement deactivation logic here...
        return Result::Success;
    }
    return Result::ConnectionError;
}
