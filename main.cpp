#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <thread>
#include "Src/Modules/TelemetryManager.h"
#include "Src/Modules/CommandManager.h"
#include "inih/cpp/INIReader.h"
#include "Events/EventManager.h"
#include "Src/Modules/CommunicationManager.h"
#include <chrono>
#include "Src/Modules/AddonsManager.h"
#include "Src/Modules/UDPVideoStreamer.h"
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;
using namespace mavsdk;
using namespace cv;
using namespace std;
class Listener {
public:
    void onEvent(int value) {
        std::cout << "Listener received event with value: " << value << std::endl;
    }

};

void freeFunctionListener(int value) {
    std::cout << "Free function listener received event with value: " << value << std::endl;
}

void usage(const std::string& bin_name) {
    std::cerr << "Usage: " << bin_name << " <connection_url>\n"
              << "Connection URL format should be:\n"
              << " For TCP: tcp://[server_host][:server_port]\n"
              << " For UDP: udp://[bind_host][:bind_port]\n"
              << " For Serial: serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}

void main_thread_function(std::shared_ptr<System> system,
                          std::shared_ptr<CommandManager> command_manager,
                          std::shared_ptr<TelemetryManager> telemetry_manager,
                          std::shared_ptr<CommunicationManager> communication_manager) {
    telemetry_manager->start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void stream_thread_function() {
    try {
        UDPVideoStreamer streamer(0, "192.168.20.21", 12345);  // Use appropriate IP and port
        streamer.stream();
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
}

bool isCameraDevice(const libusb_device_descriptor& desc) {
    // UVC cameras typically have a device class of 239, a subclass of 2, and a protocol of 1.
    // You can also specify known VIDs and PIDs if needed.
    const int CAMERA_DEVICE_CLASS = 239;  // USB Miscellaneous class (which contains the UVC class)
    const int CAMERA_DEVICE_SUBCLASS = 2;  // UVC subclass for video devices

    // Check if the device class and subclass match UVC specifications
    return (desc.bDeviceClass == CAMERA_DEVICE_CLASS && desc.bDeviceSubClass == CAMERA_DEVICE_SUBCLASS);
}

int main(int argc, char** argv) {

    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

 /*
    Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
    ConnectionResult connection_result = mavsdk.add_any_connection(argv[1]);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result << '\n';
        return 1;
    }

    // Wait for system to connect
    std::cout << "Waiting for system to connect...\n";
    bool system_discovered = false;
    mavsdk.subscribe_on_new_system([&]() {
        const auto systems = mavsdk.systems();
        if (!systems.empty()) {
            system_discovered = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(3));
    if (!system_discovered) {
        std::cerr << "Timed out waiting for system\n";
        return 1;
    }

    auto systems = mavsdk.systems();
    if (systems.empty()) {
        std::cerr << "No systems found\n";
        return 1;
    }

    auto system = systems.at(0);

    CREATE_EVENT("InfoRequest");

    auto command_manager = std::make_shared<CommandManager>(system);
    auto telemetry_manager = std::make_shared<TelemetryManager>(system);
    auto communication_manager = std::make_shared<CommunicationManager>(ECT_UDP,8080);

    communication_manager->set_command(command_manager);

    communication_manager->start();

    std::this_thread::sleep_for(std::chrono::seconds(3));


    SUBSCRIBE_TO_EVENT("InfoRequest", ([telemetry_manager, communication_manager]() {
    communication_manager->send_message_all(telemetry_manager->getTelemetryData().print());
    }));

    std::thread stream_thread(stream_thread_function);
    std::thread main_thread(main_thread_function, system, command_manager, telemetry_manager, communication_manager);


    // Print OpenCV version
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;


    main_thread.join();
    stream_thread.join();

*/
/*
     AddonsManager manager;

    // Step 2: Start detecting USB devices (in a separate thread if desired)
    manager.start();
    std::this_thread::sleep_for(std::chrono::seconds(2));  // Give some time to detect devices

    // Step 3: Identify the camera device
    int addonCount = manager.getAddonCount();
    if (addonCount == 0) {
        std::cerr << "No USB devices detected." << std::endl;
        manager.stop();  // Ensure the manager stops before exiting
        return 1;
    }

    // Search for a device that matches camera criteria
    std::shared_ptr<BaseAddon> cameraAddon = nullptr;
    for (int i = 0; i < addonCount; ++i) {
        std::shared_ptr<BaseAddon> addon = manager.getAddon(i);
        if (addon) {
            // Get the device descriptor for each addon
            libusb_device_handle* handle = addon->getDeviceHandle();
            if (!handle) {
                std::cerr << "Invalid device handle for addon at index " << i << std::endl;
                continue;
            }

            libusb_device* device = libusb_get_device(handle);
            if (!device) {
                std::cerr << "Failed to get libusb_device for addon at index " << i << std::endl;
                continue;
            }

            libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(device, &desc) == 0) {
                // Check if this device is a camera
                if (isCameraDevice(desc)) {
                    cameraAddon = addon;
                    std::cout << "Camera detected: Vendor ID: " << desc.idVendor << ", Product ID: " << desc.idProduct << std::endl;
                    break;
                }
            }
        }
    }

    // If no camera was found, stop the manager and exit cleanly
    if (!cameraAddon) {
        std::cerr << "No camera device detected." << std::endl;
        manager.stop();  // Ensure manager stops the monitoring thread
        return 1;  // Exit immediately to avoid accessing null pointers later
    }

    // Step 4: Load the JSON file containing commands
    const std::string commandFilePath = "../Src/Addons/json/webcam_commands.json";  // Path to your JSON file
    if (!cameraAddon->loadCommandsFromFile(commandFilePath)) {
        std::cerr << "Failed to load commands from JSON file: " << commandFilePath << std::endl;
        manager.stop();
        return 1;
    }

    // Step 5: Activate the camera addon
    if (cameraAddon->Activate() != BaseAddon::Result::Success) {
        std::cerr << "Failed to activate the camera." << std::endl;
        manager.stop();
        return 1;
    }

    // Step 6: Execute all commands listed in the JSON file
    std::cout << "Executing commands from JSON file..." << std::endl;
    const auto& commands = cameraAddon->getCommands();
    for (const auto& command : commands) {
        if (cameraAddon->executeCommand(command.name) != BaseAddon::Result::Success) {
            std::cerr << "Failed to execute command: " << command.name << std::endl;
        }
    }

    // Stop the monitoring thread and cleanup
    manager.stop();
    */
    int fd = open("/dev/video0", O_RDWR);
    if (fd == -1) {
        perror("Opening video device");
        return 1;
    }

    struct v4l2_control control;
    control.id = V4L2_CID_BRIGHTNESS;  // 0x00980900
    control.value = 211;  // Adjust brightness

    if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
        perror("Setting Brightness");
        return 1;
    }

    close(fd);
    return 0;
}


