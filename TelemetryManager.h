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
