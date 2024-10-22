#include "AddonsManager.h"
#include <iostream>

AddonsManager::AddonsManager() : running(false) {
    // Initialize libusb context for USB device handling
    libusb_init(nullptr);
}

AddonsManager::~AddonsManager() {
    // Stop the background thread and clean up
    stop();
    addon_ptrs.clear();
    libusb_exit(nullptr);

}

void AddonsManager::start() {
    running = true;
    monitoring_thread = std::thread(&AddonsManager::monitor_addons, this);  // Start the background monitoring thread
}

void AddonsManager::stop() {
    running = false;
    if (monitoring_thread.joinable()) {
        monitoring_thread.join();
    }
}

void AddonsManager::monitor_addons() {
    while (running) {
        detect_usb_devices();
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Check for new devices every second
    }
}

void AddonsManager::detect_usb_devices() {
    libusb_device **devices;
    libusb_context *ctx = nullptr;  // Ensure this context is initialized in the constructor
    ssize_t device_count = libusb_get_device_list(ctx, &devices);

    if (device_count < 0) {
        std::cerr << "Error getting USB device list." << std::endl;
        return;
    }

    for (ssize_t i = 0; i < device_count; ++i) {
        libusb_device *device = devices[i];
        libusb_device_descriptor desc;

        if (libusb_get_device_descriptor(device, &desc) == 0) {
            uint8_t bus = libusb_get_bus_number(device);
            uint8_t address = libusb_get_device_address(device);

            // Check if this device is already in the addon_ptrs list
            bool device_already_added = false;
            for (const auto& addon : addon_ptrs) {
                if (addon->getBusNumber() == bus && addon->getDeviceAddress() == address) {
                    device_already_added = true;
                    break;
                }
            }

            // If device is not already in the list, add it
            if (!device_already_added) {
                libusb_device_handle *handle = nullptr;
                if (libusb_open(device, &handle) == 0 && handle != nullptr) {
                    std::shared_ptr<BaseAddon> new_addon = std::make_shared<BaseAddon>("USB Addon", handle, bus, address);

                   if( new_addon->loadCommandsFromDirectory("../../Addons/json")) {
                       std::cout << "load commands succusful";
                   }

                    addon_ptrs.push_back(new_addon);
                    std::cout << "Added new USB device: " << desc.idVendor << ":" << desc.idProduct << std::endl;
                } else {
                    std::cerr << "Failed to open USB device: " << desc.idVendor << ":" << desc.idProduct << std::endl;
                }
            }
        }
    }

    libusb_free_device_list(devices, 1);  // Free the list of devices
}

int AddonsManager::getAddonCount() const {
    return addon_ptrs.size();  // Return the number of detected addons
}

std::shared_ptr<BaseAddon> AddonsManager::getAddon(int index) const {
    if (index >= 0 && index < addon_ptrs.size()) {
        return addon_ptrs[index];
    }
    return nullptr;
}
BaseAddon::Result AddonsManager::executeCommand(int index, const std::string& commandName)
{
    if (index >= 0 && index < addon_ptrs.size()) {
        BaseAddon::Result result = addon_ptrs[index]->executeCommand(commandName);
        if (result == BaseAddon::Result::Success) {
            std::cout << "Addon " << index << " activated successfully." << std::endl;
        } else {
            std::cerr << "Failed to activate addon " << index << std::endl;
        }
        return result;
    }
        std::cerr << "Invalid addon index." << std::endl;
        return BaseAddon::Result::Failure;
}

void AddonsManager::activate(int index) {
    if (index >= 0 && index < addon_ptrs.size()) {
        BaseAddon::Result result = addon_ptrs[index]->Activate();
        if (result == BaseAddon::Result::Success) {
            std::cout << "Addon " << index << " activated successfully." << std::endl;
        } else {
            std::cerr << "Failed to activate addon " << index << std::endl;
        }
    } else {
        std::cerr << "Invalid addon index." << std::endl;
    }
}

void AddonsManager::deactivate(int index) {
    if (index >= 0 && index < addon_ptrs.size()) {
        BaseAddon::Result result = addon_ptrs[index]->Deactivate();
        if (result == BaseAddon::Result::Success) {
            std::cout << "Addon " << index << " deactivated successfully." << std::endl;
        } else {
            std::cerr << "Failed to deactivate addon " << index << std::endl;
        }
    } else {
        std::cerr << "Invalid addon index." << std::endl;
    }
}
