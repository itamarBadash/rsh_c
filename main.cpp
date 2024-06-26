#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <thread>
#include <atomic>
#include "TelemetryManager.h"
#include "CommandManager.h"
#include "CommunicationManager.h"

int commandManagerTest(std::shared_ptr<mavsdk::System> system);
int TelemetryManagerTest(std::shared_ptr<System> system, std::shared_ptr<CommunicationManager> comm);

using namespace mavsdk;

void usage(const std::string& bin_name) {
    std::cerr << "Usage: " << bin_name << " <connection_url>\n"
              << "Connection URL format should be:\n"
              << " For TCP: tcp://[server_host][:server_port]\n"
              << " For UDP: udp://[bind_host][:bind_port]\n"
              << " For Serial: serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}

int TelemetryManagerTest(std::shared_ptr<System> system, std::shared_ptr<CommunicationManager> comm) {
    try {
        TelemetryManager telemetry_manager(system,comm);
        telemetry_manager.start();

        while (telemetry_manager.isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        telemetry_manager.stop();
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int commandManagerTest(std::shared_ptr<System> system) {
    CommandManager command_manager(system);

    // Test arming the system
    CommandManager::Result result = command_manager.arm();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Arm failed\n";
        return 1;
    } else {
        std::cout << "Arm successful\n";
    }

    // Test taking off
    result = command_manager.takeoff();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Takeoff failed\n";
        return 1;
    } else {
        std::cout << "Takeoff successful\n";
    }

    // Test landing
    result = command_manager.land();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Land failed\n";
        return 1;
    } else {
        std::cout << "Land successful\n";
    }

    // Test returning to launch
    result = command_manager.return_to_launch();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Return to launch failed\n";
        return 1;
    } else {
        std::cout << "Return to launch successful\n";
    }

    // Test setting flight mode
    result = command_manager.set_flight_mode(1, 2);
    if (result != CommandManager::Result::Success) {
        std::cerr << "Set flight mode failed\n";
        return 1;
    } else {
        std::cout << "Set flight mode successful\n";
    }

    // Test setting manual control
    result = command_manager.set_manual_control(0.0, 0.0, 0.5, 0.0);
    if (result != CommandManager::Result::Success) {
        std::cerr << "Set manual control failed\n";
        return 1;
    } else {
        std::cout << "Set manual control successful\n";
    }

    // Test disarming the system
    result = command_manager.disarm();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Disarm failed\n";
        return 1;
    } else {
        std::cout << "Disarm successful\n";
    }

    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

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


    auto command_manager = std::make_shared<CommandManager>(system);

    auto communication_manager = std::make_shared<CommunicationManager>("/dev/ttyUSB0",57600, command_manager);

    std::thread telemetry_thread(TelemetryManagerTest, system, communication_manager);

    telemetry_thread.join();

    return 0;
}
