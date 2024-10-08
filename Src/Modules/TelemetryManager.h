#ifndef TELEMETRY_MANAGER_H
#define TELEMETRY_MANAGER_H

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <atomic>
#include <mutex>
#include <iostream>
#include "../Communications/SerialCommunication.h"

using namespace mavsdk;
struct TelemetryData {
    Telemetry::Position position;
    Telemetry::Health health;
    Telemetry::Altitude altitude;
    Telemetry::EulerAngle euler_angle;
    Telemetry::FlightMode flight_mode;
    Telemetry::Heading heading;
    Telemetry::VelocityNed velocity;

    std::string print() const {
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
            << static_cast<int>(flight_mode) <<"\n";

        oss << "Heading: "
            << heading.heading_deg << "\n";

        oss << "Velocity NED: "
            << velocity.north_m_s << ", "
            << velocity.east_m_s << ", "
            << velocity.down_m_s << "\n";

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
    TelemetryData getTelemetryData() const;

    Telemetry::Position getLatestPosition() const;
    Telemetry::Health getLatestHealth() const;
    float getRelativeAltitude() const;
    Telemetry::EulerAngle getEulerAngle() const;
    Telemetry::FlightMode getFlightMode() const;
    Telemetry::Heading getHeading() const;
    Telemetry::VelocityNed getVelocity() const;


private:
    void subscribeTelemetry();

    std::shared_ptr<System> _system;
    std::unique_ptr<Telemetry> _telemetry;

    std::atomic<bool> _running;

    // Struct to store telemetry data
    TelemetryData _latest_telemetry_data;

    bool viable;
};

#endif // TELEMETRY_MANAGER_H