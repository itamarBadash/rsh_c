#ifndef BASE_BASEADDON_H
#define BASE_BASEADDON_H

#include <string>
#include <vector>
#include <map>
#include <libusb.h>
#include "../../Include/json.hpp"

class BaseAddon {
public:
    enum class Result {
        Success,
        Failure,
        ConnectionError,
        CommandUnsupported,
        CommandDenied,
        CommandFailed,
        UnsupportedCommandType,
        Unknown
    };

    struct Command {
        std::string name;
        std::string request_type;
        uint8_t request = 0;        // Only used for USB commands
        uint16_t value = 0;         // Only used for USB commands
        uint16_t index = 0;         // Only used for USB commands
        uint16_t data_length = 0;   // Used for both USB and ioctl
        uint32_t ioctl_code = 0;    // Used for ioctl commands
        bool is_read = false;       // Used for ioctl commands to determine read/write
        std::map<std::string, nlohmann::json> args; // Additional arguments for ioctl commands
    };

    BaseAddon(const std::string& new_name, libusb_device_handle* dev_handle, uint8_t bus, uint8_t address);

    virtual ~BaseAddon();

    uint8_t getBusNumber() const;

    uint8_t getDeviceAddress() const;

    bool loadCommandsFromDirectory(const std::string &dirPath);

    virtual Result Activate();

    virtual Result Deactivate();

    Result executeIoctlCommand(const Command &cmd);

    Result sendIoctl(uint8_t ioctl_code, uint16_t value, uint16_t index, uint8_t *data, uint16_t length);

    const std::vector<BaseAddon::Command> &getCommands() const;

    libusb_device_handle *getDeviceHandle() const;

    Result executeCommand(const std::string& commandName);

    Result executeUsbControlCommand(const Command &cmd);

protected:
    std::string name;
    libusb_device_handle* device_handle;
    uint8_t bus_number;
    uint8_t device_address;

    std::vector<Command> commands;

    uint8_t determineRequestType(const std::string& typeStr);
};

namespace nlohmann {
    template <>
    struct adl_serializer<BaseAddon::Command> {
        static void from_json(const json& j, BaseAddon::Command& cmd) {
            j.at("name").get_to(cmd.name);
            j.at("request_type").get_to(cmd.request_type);

            if (cmd.request_type == "usb") {
                j.at("request").get_to(cmd.request);
                j.at("value").get_to(cmd.value);
                j.at("index").get_to(cmd.index);
                j.at("data_length").get_to(cmd.data_length);
            } else if (cmd.request_type == "ioctl") {
                j.at("ioctl_code").get_to(cmd.ioctl_code);
                j.at("data_length").get_to(cmd.data_length);
                if (j.contains("is_read")) {
                    j.at("is_read").get_to(cmd.is_read);
                }
                if (j.contains("args")) {
                    j.at("args").get_to(cmd.args);
                }
            }
        }

        static void to_json(json& j, const BaseAddon::Command& cmd) {
            j = json{
                {"name", cmd.name},
                {"request_type", cmd.request_type},
                {"data_length", cmd.data_length}
            };

            if (cmd.request_type == "usb") {
                j["request"] = cmd.request;
                j["value"] = cmd.value;
                j["index"] = cmd.index;
            } else if (cmd.request_type == "ioctl") {
                j["ioctl_code"] = cmd.ioctl_code;
                j["is_read"] = cmd.is_read;
                j["args"] = cmd.args;
            }
        }
    };
}

#endif // BASE_BASEADDON_H