#ifndef BASE_BASEADDON_H
#define BASE_BASEADDON_H

#include <string>
#include <vector>
#include <libusb.h>
#include <../../Include/json.hpp>

class BaseAddon {
public:
    enum class Result {
        Success,
        Failure,
        ConnectionError,
        CommandUnsupported,
        CommandDenied,
        CommandFailed,
        Unknown
    };

    struct Command {
        std::string name;
        std::string request_type;
        uint8_t request;
        uint16_t value;
        uint16_t index;
        uint16_t data_length;
    };

    BaseAddon(const std::string& new_name, libusb_device_handle* dev_handle, uint8_t bus, uint8_t address);

    virtual ~BaseAddon();

    uint8_t getBusNumber() const;

    uint8_t getDeviceAddress() const;

    virtual Result Activate();

    virtual Result Deactivate();

    bool loadCommandsFromFile(const std::string& filePath);

    Result executeCommand(const std::string& commandName);

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
            j.at("request").get_to(cmd.request);
            j.at("value").get_to(cmd.value);
            j.at("index").get_to(cmd.index);
            j.at("data_length").get_to(cmd.data_length);
        }

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
