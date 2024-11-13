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

void main_thread_function(Mavsdk& mavsdk,
                          const std::string& connection_url,
                          std::shared_ptr<CommunicationManager> communication_manager) {
    bool is_connected = false;
    std::shared_ptr<CommandManager> command_manager = nullptr;
    std::shared_ptr<TelemetryManager> telemetry_manager = nullptr;

    while (true) {
        auto systems = mavsdk.systems();
        if (!systems.empty() && systems[0]->is_connected()) {
            if (!is_connected) {
                // System just connected
                std::cout << "System connected\n";
                is_connected = true;

                auto system = systems[0];

                // Initialize system-dependent managers only when system is available
                command_manager = std::make_shared<CommandManager>(system);
                telemetry_manager = std::make_shared<TelemetryManager>(system);

                // Start telemetry and communication only when connected
                telemetry_manager->start();
                communication_manager->start();

                // Subscribe to events that need telemetry_manager
                SUBSCRIBE_TO_EVENT("InfoRequest", ([telemetry_manager, communication_manager]() {
                    if (telemetry_manager) {
                        communication_manager->send_message_all(telemetry_manager->getTelemetryData().print());
                    }
                }));
            }

            // Keep telemetry and command running
            sleep_for(seconds(3));
        } else {
            if (is_connected) {
                // System just disconnected
                std::cerr << "System disconnected. Attempting to reconnect...\n";
                is_connected = false;

                // Stop telemetry and communication managers safely
                if (telemetry_manager) telemetry_manager->stop();
                communication_manager->stop();

                // Reset system-dependent managers
                command_manager = nullptr;
                telemetry_manager = nullptr;
            }

            // Attempt reconnection
            ConnectionResult connection_result = mavsdk.add_any_connection(connection_url);
            if (connection_result == ConnectionResult::Success) {
                std::cout << "Reconnection successful!\n";
            } else {
                std::cerr << "Reconnection attempt failed. Retrying in 3 seconds...\n";
                sleep_for(seconds(3));  // Retry delay
            }
        }
    }
}

void stream_thread_function() {
    try {
        UDPVideoStreamer streamer(0, "192.168.20.47", 12345);  // Use appropriate IP and port
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

    const std::string connection_url = argv[1];
    Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
    ConnectionResult connection_result = mavsdk.add_any_connection(connection_url);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Initial connection failed: " << connection_result << '\n';
    }

    // Create a manager instance that is not dependent on the system
    auto manager = make_shared<AddonsManager>();
    manager->start();

    // Initialize communication manager independently
    auto communication_manager = std::make_shared<CommunicationManager>(ECT_UDP, 8080);

    CREATE_EVENT("InfoRequest");
    CREATE_EVENT("set_brightness");

    SUBSCRIBE_TO_EVENT("set_brightness", ([manager]() {
        manager->executeCommand(1, "set_brightness");
    }));

    // Start the streaming thread (runs independently)
    std::thread stream_thread(stream_thread_function);

    // Start the main thread to manage system connection and reconnection
    std::thread main_thread(main_thread_function, std::ref(mavsdk), connection_url, communication_manager);


    main_thread.join();
    stream_thread.join();
    manager->stop();

    return 0;
}
