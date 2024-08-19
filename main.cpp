#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <thread>
#include "Src/Modules/TelemetryManager.h"
#include "Src/Modules/CommandManager.h"
#include "Src/Communications/SerialCommunication.h"
#include "inih/cpp/INIReader.h"
#include "Src/Modules/BaseAddon.h"
#include "Events/EventManager.h"
#include "Src/Communications/TCPServer.h"
#include "Src/Communications/UDPServer.h"
#include "Src/Modules/CommunicationManager.h"
#include <chrono>
using std::chrono::seconds;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;
using namespace mavsdk;

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
                          std::shared_ptr<TelemetryManager> telemetry_manager, std::shared_ptr<CommunicationManager> communication_manager) {
    telemetry_manager->start();

    CREATE_EVENT("InfoRequest");

    SUBSCRIBE_TO_EVENT("InfoRequest", ([telemetry_manager, communication_manager]() {
    communication_manager->send_message(telemetry_manager->getTelemetryData().print());
    }));

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
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
    auto telemetry_manager = std::make_shared<TelemetryManager>(system);
    auto communication_manager = std::make_shared<CommunicationManager>(ECT_UDP);

    communication_manager->set_command(command_manager);

    communication_manager->start();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    if (command_manager->handle_command("arm", {}) != CommandManager::Result::Success) {
        std::cerr << "Failed to arm the drone" << std::endl;
        return 1;
    }

    // Start manual control mode
    if (command_manager->start_manual_control() != CommandManager::Result::Success) {
        std::cerr << "Failed to start manual control" << std::endl;
        return 1;
    }

    // Wait for a short moment to ensure manual control is engaged

    float ascent_speed = 0.7f; // Adjust this value for a reasonable ascent speed

    // Ascend for a few seconds
    std::cout << "Ascending...\n";
    for (int i = 0; i < 50; ++i) { // Adjust loop count or duration as needed
        command_manager->provide_control_input(0.0f, 0.0f, ascent_speed, 0.0f);
        sleep_for(milliseconds(100)); // Send input every 100 milliseconds
    }

    // Stop ascent and hover
    std::cout << "Hovering...\n";
    for (int i = 0; i < 50; ++i) { // Adjust loop count or duration as needed
        command_manager->provide_control_input(0.0f, 0.0f, 0.5f, 0.0f); // 0.5f typically represents hover
        sleep_for(milliseconds(100)); // Send input every 100 milliseconds
    }

    auto addon = std::make_shared<BaseAddon>("system");

   // std::thread main_thread(main_thread_function, system, command_manager,telemetry_manager,communication_manager);

   // main_thread.join();

    return 0;
}


