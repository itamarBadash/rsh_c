#ifndef TELEMETRY_MANAGER_H
#define TELEMETRY_MANAGER_H

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <atomic>
#include <mutex>
#include <iostream>

using namespace mavsdk;
struct TelemetryData {
    Telemetry::Position position;
    Telemetry::Health health;
    Telemetry::Altitude altitude;
    Telemetry::EulerAngle euler_angle;
    Telemetry::FlightMode flight_mode;
    Telemetry::Heading heading;
    Telemetry::VelocityNed velocity;

    std::string print(const TelemetryManager &telemetry_manager) const {
        std::ostringstream oss;
        oss << "Position: "
            << position.latitude_deg << ", "
            << position.longitude_deg << ", "
            << position.relative_altitude_m << ", "
            << position.absolute_altitude_m << "\n";

        oss << "Health: "
            << "Gyro: " << (health.is_gyrometer_calibration_ok ? "OK" : "Not OK") << ", "
            << "Acc: " << (health.is_accelerometer_calibration_ok ? "OK" : "Not OK") << ", "
            << "Mag: " << (health.is_magnetometer_calibration_ok ? "OK" : "Not OK") << "\n";

        oss << "Euler Angles: "
            << euler_angle.roll_deg << ", "
            << euler_angle.pitch_deg << ", "
            << euler_angle.yaw_deg << "\n";

        oss << "Flight Mode: "
            << flight_mode.mode << "\n";

        oss << "Heading: "
            << heading.heading_deg << "\n";

        oss << "Velocity NED: "
            << velocity.north_m_s << ", "
            << velocity.east_m_s << ", "
            << velocity.down_m_s << "\n";

        oss << "Altitude: "
            << telemetry_manager.getRelativeAltitude() << "\n";

        return oss.str();
    }
};

class TelemetryManager {
public:
    TelemetryManager(const std::shared_ptr<System>& system);
    ~TelemetryManager();

    void start();
    void stop();
    bool isRunning() { return _running; }

    // Methods to get the latest telemetry data
    TelemetryData getTelemetryData();

    Telemetry::Position getLatestPosition();
    Telemetry::Health getLatestHealth();
    float getRelativeAltitude();
    Telemetry::EulerAngle getEulerAngle();
    Telemetry::FlightMode getFlightMode();
    Telemetry::Heading getHeading();
    Telemetry::VelocityNed getVelocity();


private:
    void subscribeTelemetry();

    std::shared_ptr<System> _system;
    std::unique_ptr<Telemetry> _telemetry;

    std::atomic<bool> _running;

    // Struct to store telemetry data
    TelemetryData _latest_telemetry_data;

    std::mutex _data_mutex;
    bool viable;

};

#endif // TELEMETRY_MANAGER_H
