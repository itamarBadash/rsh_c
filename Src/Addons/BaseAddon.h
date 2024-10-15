#ifndef BASE_BASEADDON_H
#define BASE_BASEADDON_H

#include <string>
#include <libusb.h>

class BaseAddon {
public:
    // Enum for result status
    enum class Result {
        Success,
        Failure,
        ConnectionError,
        CommandUnsupported,
        CommandDenied,
        CommandFailed,
        Unknown
    };

    // Constructor that accepts a name for the addon and an optional USB device handle
    BaseAddon(const std::string& new_name, libusb_device_handle* dev_handle = nullptr);

    // Virtual destructor to allow proper cleanup of derived classes
    virtual ~BaseAddon();

    // Getter for the name of the addon
    std::string getName() const;

    // Virtual function to activate the addon (can be overridden by derived classes)
    virtual Result Activate();

    // Virtual function to deactivate the addon (to be overridden by derived classes)
    virtual Result Deactivate();

    // Additional methods for controlling device status
    virtual Result GetStatus();

    // USB control transfer for sending custom commands to the device
    virtual Result SendCommand(uint8_t request, uint16_t value, uint16_t index);

protected:
    std::string name;  // Name of the addon
    libusb_device_handle* device_handle;  // Handle to the connected USB device
};

#endif //BASE_BASEADDON_H
