#include "BaseAddon.h"
#include <iostream>
#include <fstream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <linux/videodev2.h>

using namespace nlohmann;

BaseAddon::BaseAddon(const std::string& new_name, libusb_device_handle* dev_handle, uint8_t bus, uint8_t address)
    : name(new_name), device_handle(dev_handle), bus_number(bus), device_address(address) {
}

BaseAddon::~BaseAddon() {
    if (device_handle) {
        libusb_close(device_handle);
        device_handle = nullptr;
    }
}

uint8_t BaseAddon::getBusNumber() const {
    return bus_number;
}

uint8_t BaseAddon::getDeviceAddress() const {
    return device_address;
}

// Load commands from a JSON file
bool BaseAddon::loadCommandsFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open command file: " << filePath << std::endl;
        return false;
    }

    try {
        json jsonData;
        file >> jsonData;
        commands = jsonData["commands"].get<std::vector<Command>>();  // Assuming you have a Command struct defined
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Execute a command based on the command name
BaseAddon::Result BaseAddon::executeCommand(const std::string& commandName) {
    auto it = std::find_if(commands.begin(), commands.end(), [&](const Command& cmd) {
        return cmd.name == commandName;
    });

    if (it != commands.end()) {
        if (it->request_type == "usb") {
            return executeUsbControlCommand(*it);
        } else if (it->request_type == "ioctl") {
            return executeIoctlCommand(*it);
        } else {
            std::cerr << "Unsupported request type: " << it->request_type << std::endl;
            return Result::UnsupportedCommandType;
        }
    }

    std::cerr << "Command '" << commandName << "' not found." << std::endl;
    return Result::Failure;
}

// Helper method to execute USB control commands
BaseAddon::Result BaseAddon::executeUsbControlCommand(const Command& cmd) {
    uint8_t requestType = determineRequestType(cmd.request_type);

    // Prepare buffer if the command specifies a data length
    std::vector<uint8_t> buffer(cmd.data_length, 0);
    uint8_t* buffer_ptr = (cmd.data_length > 0) ? buffer.data() : nullptr;

    int result = libusb_control_transfer(
        device_handle,
        requestType,
        cmd.request,
        cmd.value,
        cmd.index,
        buffer_ptr,
        cmd.data_length,
        1000  // Timeout in milliseconds
    );

    if (result < 0) {
        std::cerr << "Failed to execute USB control command '" << cmd.name << "': " << libusb_error_name(result) << std::endl;
        return Result::Failure;
    }

    std::cout << "Executed USB control command '" << cmd.name << "' successfully." << std::endl;
    return Result::Success;
}

// Helper method to determine request type from a string
uint8_t BaseAddon::determineRequestType(const std::string& typeStr) {
    if (typeStr == "vendor") {
        return LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE;
    } else if (typeStr == "standard") {
        return LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE;
    }
    // Add more cases if needed
    return 0;
}

// Activate the device and get device status
BaseAddon::Result BaseAddon::Activate() {
    if (device_handle) {
        std::cout << "Activating " << name << " on bus " << (int)bus_number << " address " << (int)device_address << std::endl;

        // Get and print device descriptor information
        libusb_device_descriptor desc;
        int result = libusb_get_device_descriptor(libusb_get_device(device_handle), &desc);
        if (result == 0) {
            std::cout << "Device Class: " << (int)desc.bDeviceClass << std::endl;
            std::cout << "Vendor ID: " << desc.idVendor << std::endl;
            std::cout << "Product ID: " << desc.idProduct << std::endl;
        } else {
            std::cerr << "Failed to get device descriptor: " << libusb_error_name(result) << std::endl;
            return Result::Failure;
        }

        // Prepare buffer to hold status
        uint8_t buffer[2];  // Buffer to hold the status

        // Send a standard GET_STATUS request
        result = libusb_control_transfer(device_handle,
                                         LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
                                         LIBUSB_REQUEST_GET_STATUS,  // Standard GET_STATUS request
                                         0,  // wValue (not used for GET_STATUS)
                                         0,  // wIndex (0 for device-wide status)
                                         buffer,  // Buffer to store status
                                         sizeof(buffer),  // Size of the buffer (2 bytes)
                                         1000);  // Timeout in milliseconds

        if (result < 0) {
            std::cerr << "Failed to send GET_STATUS: " << libusb_error_name(result) << std::endl;
            return Result::Failure;
        }

        // Parse the status
        uint16_t status = buffer[0] | (buffer[1] << 8);
        std::cout << "Device status: " << status << std::endl;

        return Result::Success;
    }
    return Result::ConnectionError;
}

// Deactivate and reset the device
BaseAddon::Result BaseAddon::Deactivate() {
    if (device_handle) {
        std::cout << "Resetting USB device " << name << " on bus " << (int)bus_number << " address " << (int)device_address << std::endl;

        // Reset the USB device
        int result = libusb_reset_device(device_handle);
        if (result < 0) {
            std::cerr << "Failed to reset device: " << libusb_error_name(result) << std::endl;
            return Result::Failure;
        }

        std::cout << "Device reset successfully." << std::endl;
        return Result::Success;
    }
    return Result::ConnectionError;
}

// Define the ioctl-like function to send custom control codes
BaseAddon::Result BaseAddon::executeIoctlCommand(const Command &cmd) {
    std::string deviceFile = cmd.args.at("fd").get<std::string>();
    std::cout << "Opening device file: " << deviceFile << std::endl;

    int fd = open(deviceFile.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open device file: " << strerror(errno) << std::endl;
        return Result::Failure;
    }

    std::cout << "Device file opened successfully." << std::endl;

    struct v4l2_control control;
    control.id = cmd.args.at("control_id").get<int>();
    control.value = cmd.args.at("value").get<int>();

    std::cout << "Executing ioctl command with control ID: " << control.id << " and value: " << control.value << std::endl;

    int result = ioctl(fd, VIDIOC_S_CTRL, &control);

    close(fd);

    if (result < 0) {
        std::cerr << "Failed to execute ioctl command: " << strerror(errno) << " (errno: " << errno << ")" << std::endl;
        return Result::Failure;
    }

    std::cout << "Ioctl command executed successfully." << std::endl;

    return Result::Success;
}const std::vector<BaseAddon::Command>& BaseAddon::getCommands() const {
    return commands;
}

libusb_device_handle* BaseAddon::getDeviceHandle() const {
    return device_handle;
}