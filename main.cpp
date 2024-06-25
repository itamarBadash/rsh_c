#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <thread>
#include <atomic>
#include "TelemetryManager.h"
#include "CommandManager.h"
#include "CommunicationManager.h"

int TelemetryManagerTest( std::shared_ptr<System>& system);
int commandManagerTest( std::shared_ptr<mavsdk::System> system);
using namespace mavsdk;


void usage(const std::string& bin_name) {
    std::cerr << "Usage: " << bin_name << " <connection_url>\n"
              << "Connection URL format should be:\n"
              << " For TCP: tcp://[server_host][:server_port]\n"
              << " For UDP: udp://[bind_host][:bind_port]\n"
              << " For Serial: serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}

int TelemetryManagerTest( std::shared_ptr<System>& system) {
    try {
        TelemetryManager telemetry_manager(system);
        telemetry_manager.start();

        while (telemetry_manager.isRunning()) {
            TelemetryData data = telemetry_manager.getTelemetryData();

            std::cout << "Position: "
                      << data.position.latitude_deg << ", "
                      << data.position.longitude_deg << ", "
                      << data.position.relative_altitude_m << ", "
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
            std::cout << "Altitude: "
                      << telemetry_manager.getRelativeAltitude() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        telemetry_manager.stop();
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
int commandManagerTest(std::shared_ptr<mavsdk::System> system){

    CommandManager commandManager(system);

    // Test arming the system
    CommandManager::Result result = commandManager.arm();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Arm failed\n";
        return 1;
    } else {
        std::cout << "Arm successful\n";
    }

    // Test taking off
    result = commandManager.takeoff();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Takeoff failed\n";
        return 1;
    } else {
        std::cout << "Takeoff successful\n";
    }

    // Test landing
    result = commandManager.land();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Land failed\n";
        return 1;
    } else {
        std::cout << "Land successful\n";
    }

    // Test returning to launch
    result = commandManager.return_to_launch();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Return to launch failed\n";
        return 1;
    } else {
        std::cout << "Return to launch successful\n";
    }

    // Test setting flight mode
    result = commandManager.set_flight_mode(1, 2);
    if (result != CommandManager::Result::Success) {
        std::cerr << "Set flight mode failed\n";
        return 1;
    } else {
        std::cout << "Set flight mode successful\n";
    }

    // Test setting manual control
    result = commandManager.set_manual_control(0.0, 0.0, 0.5, 0.0);
    if (result != CommandManager::Result::Success) {
        std::cerr << "Set manual control failed\n";
        return 1;
    } else {
        std::cout << "Set manual control successful\n";
    }

    // Test disarming the system
    result = commandManager.disarm();
    if (result != CommandManager::Result::Success) {
        std::cerr << "Disarm failed\n";
        return 1;
    } else {
        std::cout << "Disarm successful\n";
    }

    return 0;
}


int main(int argc, char** argv)
{
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

    auto system = mavsdk.first_autopilot(3.0);
    if (!system) {
        std::cerr << "Timed out waiting for system\n";
        return 1;
    }
    auto current_system = system.value();

    CommunicationManager communicationManager("/dev/ttyUSB0",57600);

    TelemetryManagerTest(current_system);
    while (true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}