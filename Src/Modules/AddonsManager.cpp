#include "AddonsManager.h"
#include <iostream>

AddonsManager::AddonsManager() : running(false) {
    // Initialize libusb context for USB device handling
    libusb_init(nullptr);
}

AddonsManager::~AddonsManager() {
    // Stop the background thread and clean up
    running = false;
    if (monitoring_thread.joinable()) {
        monitoring_thread.join();
    }
    libusb_exit(nullptr); // Clean up libusb context
}

void AddonsManager::start() {
    running = true;
    monitoring_thread = std::thread(&AddonsManager::monitor_addons, this); // Start the background monitoring thread
}

void AddonsManager::monitor_addons() {
    while (running) {
        detect_usb_devices();
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Check for new devices every second
    }
}

void AddonsManager::detect_usb_devices() {
    libusb_device **devices;
    libusb_context *ctx = nullptr;  // This context must be initialized earlier in the AddonsManager (or in start)
    ssize_t device_count = libusb_get_device_list(ctx, &devices);

    if (device_count < 0) {
        std::cerr << "Error getting USB device list." << std::endl;
        return;
    }

    for (ssize_t i = 0; i < device_count; ++i) {
        libusb_device *device = devices[i];
        libusb_device_descriptor desc;

        if (libusb_get_device_descriptor(device, &desc) == 0) {
            libusb_device_handle *handle = nullptr;

            if (libusb_open(device, &handle) == 0 && handle != nullptr) {
                std::shared_ptr<BaseAddon> new_addon = std::make_shared<BaseAddon>("USB Addon", handle);
                addon_ptrs.push_back(new_addon);
            } else {
                std::cerr << "Failed to open USB device: " << desc.idVendor << ":" << desc.idProduct << std::endl;
            }
        }
    }

    libusb_free_device_list(devices, 1);
}

int AddonsManager::getAddonCount() const {
    return addon_ptrs.size(); // Return the number of detected addons
}

void AddonsManager::activate(int index) {
    if (index >= 0 && index < addon_ptrs.size()) {
        BaseAddon::Result result = addon_ptrs[index]->Activate();
        if (result == BaseAddon::Result::Success) {
            std::cout << "Addon " << index << " activated successfully." << std::endl;
        } else {
            std::cerr << "Failed to activate addon " << index << std::endl;
        }
    }else {
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
