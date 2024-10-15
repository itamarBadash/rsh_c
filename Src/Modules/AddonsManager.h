#ifndef ADDONSMANAGER_H
#define ADDONSMANAGER_H

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <libusb.h>

#include "../Addons/BaseAddon.h"

class AddonsManager {
public:
    AddonsManager();
    ~AddonsManager();

    // Start monitoring and managing addons
    void start();

    // Activate a specific addon by index
    void activate(int index);

    // Deactivate a specific addon by index
    void deactivate(int index);

    // Method to return the number of currently detected addons
    int getAddonCount() const;

private:
    std::vector<std::shared_ptr<BaseAddon>> addon_ptrs; // Store the addons
    std::thread monitoring_thread; // Thread for monitoring USB devices
    std::atomic<bool> running; // To control the background thread

    // Helper function to monitor connected USB devices and update the addon list
    void monitor_addons();

    // Function to detect USB devices and add them as addons
    void detect_usb_devices();
};

#endif // ADDONSMANAGER_H
