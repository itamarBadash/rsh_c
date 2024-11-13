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
        sleep_for(std::chrono::seconds(3));
    }
}

void stream_thread_function() {
    try {
        UDPVideoStreamer streamer(0, "192.168.20.10", 12345);  // Use appropriate IP and port
        streamer.stream();
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
}

bool ConnectToSystem(const char *connection_url, std::shared_ptr<System> &system, Mavsdk &mavsdk) {

    ConnectionResult connection_result = mavsdk.add_any_connection(connection_url);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result << '\n';
        return false;
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
        return false;
    }

    auto systems = mavsdk.systems();
    if (systems.empty()) {
        std::cerr << "No systems found\n";
        return false;
    }

    system = systems.at(0);
    return true;
}

void reconnect_loop(const char* connection_url, std::shared_ptr<System>& system, Mavsdk& mavsdk) {
    while (!ConnectToSystem(connection_url, system, mavsdk)) {
        std::cerr << "Reconnection attempt failed. Retrying in 5 seconds...\n";
        sleep_for(std::chrono::seconds(5)); // Wait before retrying
    }
    std::cout << "Successfully connected to system.\n";
}

int main(int argc, char** argv) {

    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    // Initialize Mavsdk outside of ConnectToSystem to avoid re-initialization
    Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
    std::shared_ptr<System> system;

    CREATE_EVENT("InfoRequest");
    CREATE_EVENT("set_brightness");

    auto manager = std::make_shared<AddonsManager>();
    manager->start();

    sleep_for(std::chrono::seconds(1));
    SUBSCRIBE_TO_EVENT("set_brightness", [manager]() {
        manager->executeCommand(1, "set_brightness");
    });

    auto communication_manager = std::make_shared<CommunicationManager>(ECT_UDP, 8080);
    communication_manager->start();

    // Stream thread is not joined if ConnectToSystem fails to prevent blocking
    std::thread stream_thread(stream_thread_function);

    // Attempt to connect and keep reconnecting if connection is lost
    std::thread connection_thread([&]() {
        reconnect_loop(argv[1], system, mavsdk);
    });

    auto command_manager = std::make_shared<CommandManager>(system);
    auto telemetry_manager = std::make_shared<TelemetryManager>(system);

    communication_manager->set_command(command_manager);

    sleep_for(std::chrono::seconds(3));

    SUBSCRIBE_TO_EVENT("InfoRequest", [=]() {
        communication_manager->send_message_all(telemetry_manager->getTelemetryData().print());
    });

    // Main thread for telemetry
    std::thread main_thread(main_thread_function, system, command_manager, telemetry_manager, communication_manager);

    // Ensure proper shutdown by joining threads and stopping the manager
    connection_thread.join();
    main_thread.join();
    stream_thread.join();
    manager->stop();

    return 0;
}
