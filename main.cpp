#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <thread>
#include <atomic>
#include "TelemetryManager.h"


using namespace mavsdk;

std::atomic<bool> keep_running{true}; // Atomic flag to control the loop
std::atomic<double> latitude{0.0};
std::atomic<double> longitude{0.0};
std::atomic<float> battery_percentage{0.0};

void initialize_telemetry(Telemetry& telemetry) {
    // Fetch initial position
    Telemetry::Position position = telemetry.position();
    latitude.store(position.latitude_deg);
    longitude.store(position.longitude_deg);

    // Fetch initial battery status
    Telemetry::Battery battery = telemetry.battery();
    battery_percentage.store(battery.remaining_percent * 100);
}

void telemetry_thread_function(Telemetry& telemetry) {
    // Initialize telemetry data
    initialize_telemetry(telemetry);

    // Subscribe to position updates
    telemetry.subscribe_position([](Telemetry::Position position) {
        latitude.store(position.latitude_deg);
        longitude.store(position.longitude_deg);
    });

    // Subscribe to battery updates
    telemetry.subscribe_battery([](Telemetry::Battery battery) {
        battery_percentage.store(battery.remaining_percent * 100);
    });

    // Loop to keep the thread alive and print the latest telemetry data
    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "Latitude: " << latitude.load() << ", Longitude: " << longitude.load() << std::endl;
    }
}

void usage(const std::string& bin_name) {
    std::cerr << "Usage: " << bin_name << " <connection_url>\n"
              << "Connection URL format should be:\n"
              << " For TCP: tcp://[server_host][:server_port]\n"
              << " For UDP: udp://[bind_host][:bind_port]\n"
              << " For Serial: serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}

int main(int argc, char** argv) {
    if (argc != 2) { // Ensure correct usage
        usage(argv[0]);
        return 1;
    }

    // Initialize MAVSDK
    Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::CompanionComputer}};
    ConnectionResult connection_result = mavsdk.add_any_connection(argv[1]);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result << '\n';
        return 1;
    }

    // Wait for the first autopilot system to connect
    auto system = mavsdk.first_autopilot(3.0);
    if (!system) {
        std::cerr << "Timed out waiting for system\n";
        return 1;
    }

    TelemetryManager telemetryManager{mavsdk};
    // Create the Telemetry object
    Telemetry telemetry{system.value()};
    

    // Start the telemetry thread
    std::thread telemetry_thread(telemetry_thread_function, std::ref(telemetry));

    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get(); // Wait for user input

    // Stop the telemetry thread
    keep_running = false;
    telemetry_thread.join(); // Wait for the thread to finish

    return 0;
}
