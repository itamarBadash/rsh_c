#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include "Src/Modules/TelemetryManager.h"
#include "Src/Modules/CommandManager.h"
#include "inih/cpp/INIReader.h"
#include "Events/EventManager.h"
#include "Src/Modules/CommunicationManager.h"
#include "Src/Modules/AddonsManager.h"
#include "Src/Modules/UDPVideoStreamer.h"
#include <mutex>
#include <condition_variable>
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

std::shared_ptr<System> connect_to_system(const char* connection_url) {
    std::shared_ptr<System> system;
    std::mutex mutex;
    std::condition_variable cv;
    bool system_discovered = false;

    while (true) {
        Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};

        ConnectionResult connection_result = mavsdk.add_any_connection(connection_url);
        if (connection_result != ConnectionResult::Success) {
            std::cerr << "Connection failed: " << connection_result << '\n';
            sleep_for(seconds(5)); // Wait before retrying
            continue;
        }

        std::cout << "Waiting for system to connect...\n";

        mavsdk.subscribe_on_new_system([&]() {
            std::lock_guard<std::mutex> lock(mutex);
            system_discovered = true;
            cv.notify_one();
        });

        {
            std::unique_lock<std::mutex> lock(mutex);
            if (!cv.wait_for(lock, seconds(10), [&] { return system_discovered; })) {
                std::cerr << "Timed out waiting for system\n";
                continue;
            }
        }

        auto systems = mavsdk.systems();
        if (!systems.empty()) {
            system = systems.at(0);
            std::cout << "System connected successfully.\n";
            break;
        } else {
            std::cerr << "No systems found\n";
        }

        sleep_for(seconds(5)); // Retry after a delay
    }

    return system;
}

void monitor_and_reconnect(std::shared_ptr<System>& system, const char* connection_url, std::shared_ptr<CommandManager>& command_manager, std::shared_ptr<TelemetryManager>& telemetry_manager) {
    try {
        while (true) {
            if (!system || !system->is_connected()) {
                std::cerr << "System disconnected or not available. Attempting to reconnect...\n";
                system = connect_to_system(connection_url);

                if (system) {
                    command_manager = std::make_shared<CommandManager>(system);
                    telemetry_manager = std::make_shared<TelemetryManager>(system);
                    std::cout << "Reinitialized CommandManager and TelemetryManager after reconnection.\n";
                }
            }
            sleep_for(seconds(3)); // Check the connection status periodically
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in monitor_and_reconnect: " << e.what() << std::endl;
    }
}

void stream_thread_function() {
    try {
        UDPVideoStreamer streamer(0, "192.168.20.11", 12345);  // Use appropriate IP and port
        streamer.stream();
    } catch (const std::exception& ex) {
        std::cerr << "Exception in stream_thread_function: " << ex.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            usage(argv[0]);
            return 1;
        }

        CREATE_EVENT("InfoRequest");
        CREATE_EVENT("set_brightness");

        auto manager = std::make_shared<AddonsManager>();
        manager->start();

        sleep_for(std::chrono::seconds(1));
        SUBSCRIBE_TO_EVENT("set_brightness", [manager]() {
            manager->executeCommand(1, "set_brightness");
        });

        auto communication_manager = std::make_shared<CommunicationManager>(ECT_UDP, 8080);
        communication_manager->start(); // Start independently of system connection

        // Start the stream thread independently of the system connection
        std::thread stream_thread(stream_thread_function);

        // Attempt to connect and monitor the connection status independently
        std::shared_ptr<System> system = connect_to_system(argv[1]);
        std::shared_ptr<CommandManager> command_manager;
        std::shared_ptr<TelemetryManager> telemetry_manager;

        // Start monitoring and reconnecting in case of disconnection
        std::thread reconnect_thread(monitor_and_reconnect, std::ref(system), argv[1], std::ref(command_manager), std::ref(telemetry_manager));

        // Set the command manager dynamically once the system is connected
        if (system) {
            command_manager = std::make_shared<CommandManager>(system);
            telemetry_manager = std::make_shared<TelemetryManager>(system);
            communication_manager->set_command(command_manager);
        }

        sleep_for(std::chrono::seconds(3));

        SUBSCRIBE_TO_EVENT("InfoRequest", [=]() {
            if (telemetry_manager) {
                communication_manager->send_message_all(telemetry_manager->getTelemetryData().print());
            }
        });

        // Ensure proper shutdown by joining threads and stopping the manager
        stream_thread.join();
        reconnect_thread.detach(); // Allow reconnect thread to run independently
        manager->stop();

    } catch (const std::system_error& e) {
        std::cerr << "System error in main: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception in main: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error in main." << std::endl;
        return 1;
    }

    return 0;
}
