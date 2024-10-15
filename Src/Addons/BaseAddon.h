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

    // Constructor that accepts a name, device handle, bus number, and device address
    BaseAddon(const std::string& new_name, libusb_device_handle* dev_handle, uint8_t bus, uint8_t address);

    // Virtual destructor to allow proper cleanup
    virtual ~BaseAddon();

    // Getter for the bus number
    uint8_t getBusNumber() const;

    // Getter for the device address
    uint8_t getDeviceAddress() const;

    // Virtual function to activate the addon
    virtual Result Activate();

    // Virtual function to deactivate the addon
    virtual Result Deactivate();

protected:
    std::string name;
    libusb_device_handle* device_handle;
    uint8_t bus_number;  // Bus number of the USB device
    uint8_t device_address;  // Address of the USB device
};

#endif // BASE_BASEADDON_H
