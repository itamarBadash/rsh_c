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
    void stop();
    int getAddonCount() const;
    std::shared_ptr<BaseAddon> getAddon(int index) const;

    BaseAddon::Result executeCommand(int index, const std::string &commandName);

    void activate(int index);
    void deactivate(int index);

private:
    void monitor_addons();
    void detect_usb_devices();

    bool running;
    std::thread monitoring_thread;
    std::vector<std::shared_ptr<BaseAddon>> addon_ptrs;
};

#endif // ADDONSMANAGER_H

