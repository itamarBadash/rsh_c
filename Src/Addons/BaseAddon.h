#ifndef BASE_BASEADDON_H
#define BASE_BASEADDON_H

#include <string>
#include <vector>
#include <libusb.h>
#include <../../include/json.hpp>  // Include nlohmann JSON for parsing

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

    // Structure to define a command
    struct Command {
        std::string name;
        std::string request_type;
        uint8_t request;
        uint16_t value;
        uint16_t index;
        uint16_t data_length;
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

    // Function to load commands from a JSON file
    bool loadCommandsFromFile(const std::string& filePath);

    // Function to execute a command by name
    Result executeCommand(const std::string& commandName);

protected:
    std::string name;
    libusb_device_handle* device_handle;
    uint8_t bus_number;  // Bus number of the USB device
    uint8_t device_address;  // Address of the USB device

    // Vector to store the list of commands
    std::vector<Command> commands;

    // Helper function to determine the request type from a string
    uint8_t determineRequestType(const std::string& typeStr);
};

// Specialization for JSON serialization and deserialization
namespace nlohmann {
    template <>
    struct adl_serializer<BaseAddon::Command> {
        // Function to convert JSON to Command struct
        static void from_json(const json& j, BaseAddon::Command& cmd) {
            j.at("name").get_to(cmd.name);
            j.at("request_type").get_to(cmd.request_type);
            j.at("request").get_to(cmd.request);
            j.at("value").get_to(cmd.value);
            j.at("index").get_to(cmd.index);
            j.at("data_length").get_to(cmd.data_length);
        }

        // Function to convert Command struct to JSON
        static void to_json(json& j, const BaseAddon::Command& cmd) {
            j = json{
                {"name", cmd.name},
                {"request_type", cmd.request_type},
                {"request", cmd.request},
                {"value", cmd.value},
                {"index", cmd.index},
                {"data_length", cmd.data_length}
            };
        }
    };
}

#endif // BASE_BASEADDON_H
