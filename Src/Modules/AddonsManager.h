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

    void start();
    void activate(int index);
    void deactivate(int index);
    int getAddonCount() const;

private:
    std::vector<std::shared_ptr<BaseAddon>> addon_ptrs;
    std::thread monitoring_thread;
    std::atomic<bool> running;

    void monitor_addons();
    void detect_usb_devices();
};

#endif // ADDONSMANAGER_H
