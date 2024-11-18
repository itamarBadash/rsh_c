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
#include <fcntl.h>

#include <stdio.h>

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;
using namespace mavsdk;
using namespace cv;
using namespace std;

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
        UDPVideoStreamer streamer(0, "192.168.20.11", 12345);  // Use appropriate IP and port
        streamer.stream();
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
}

int main(int argc, char** argv) {

    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }
    auto communication_manager = std::make_shared<CommunicationManager>(ECT_TCP,8080);
    communication_manager->start();

    CREATE_EVENT("InfoRequest");
    CREATE_EVENT("set_brightness");
    CREATE_EVENT("command_received", const std::string & command, const std::vector<float> & parameters);


    std::thread stream_thread(stream_thread_function);

    auto manager = make_shared<AddonsManager>();
    manager->start();

    sleep_for(std::chrono::seconds(1));
    SUBSCRIBE_TO_EVENT("set_brightness", ([manager]() {
        manager->executeCommand(1,"set_brightness");
    }));

    Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};

    while (mavsdk.add_any_connection(argv[1]) != ConnectionResult::Success) {
        sleep_for(seconds(5));
        std::cerr << "Connection failed: " << '\n';
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

    sleep_for(seconds(3));
    while (!system_discovered) {
        sleep_for(seconds(10));
        std::cerr << "Timed out waiting for system\n";
    }

    auto systems = mavsdk.systems();
    while (systems.empty()) {
        std::cerr << "No systems found\n";
    }

    auto system = systems.at(0);


    auto command_manager = std::make_shared<CommandManager>(system);
    auto telemetry_manager = std::make_shared<TelemetryManager>(system);

   sleep_for(std::chrono::seconds(3));


    SUBSCRIBE_TO_EVENT("InfoRequest", ([telemetry_manager, communication_manager]() {
    communication_manager->send_message_all(telemetry_manager->getTelemetryData().print());
    }));

    SUBSCRIBE_TO_EVENT("command_received", [command_manager](const std::string& command, const std::vector<float>& parameters) {
        if (command_manager != nullptr && command_manager->IsViable()) {
            if (command_manager->is_command_valid(command)){
            auto result = command_manager->handle_command(command, parameters);
            if(result != CommandManager::Result::Success)
                cerr << "Command Failed" << std::endl;
        }else {
                std::cerr << "Invalid command: " << command << std::endl;
            }
        }
        else {
            std::cerr << "Command manager not set or not viable." << std::endl;
        }
    });


    std::thread main_thread(main_thread_function, system, command_manager, telemetry_manager, communication_manager);

    main_thread.join();
    stream_thread.join();
    manager->stop();


    return 0;
}