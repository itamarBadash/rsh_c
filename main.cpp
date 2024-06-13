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

    try {
        TelemetryManager telemetry_manager(mavsdk);
        telemetry_manager.start();

        while (true) {
            TelemetryData data = telemetry_manager.getTelemetryData();

            std::cout << "Position: "
                      << data.position.latitude_deg << ", "
                      << data.position.longitude_deg << ", "
                      << data.position.absolute_altitude_m << std::endl;

            std::cout << "Health: "
                      << "Gyro: " << data.health.is_gyrometer_calibration_ok << ", "
                      << "Acc: " << data.health.is_accelerometer_calibration_ok << ", "
                      << "Mag: " << data.health.is_magnetometer_calibration_ok << ", "
                      <<  std::endl;

            std::cout << "Euler Angles: "
                      << data.euler_angle.roll_deg << ", "
                      << data.euler_angle.pitch_deg << ", "
                      << data.euler_angle.yaw_deg << std::endl;

            std::cout << "Flight Mode: "
                      << static_cast<int>(data.flight_mode) << std::endl;

            std::cout << "Heading: "
                      << data.heading.heading_deg << std::endl;

            std::cout << "Velocity NED: "
                      << data.velocity.north_m_s << ", "
                      << data.velocity.east_m_s << ", "
                      << data.velocity.down_m_s << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        telemetry_manager.stop();
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
