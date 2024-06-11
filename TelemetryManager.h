#ifndef TELEMETRY_MANAGER_H
#define TELEMETRY_MANAGER_H

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <atomic>
#include <mutex>
#include <iostream>

using namespace mavsdk;

class TelemetryManager {
public:
    TelemetryManager(Mavsdk& mavsdk);
    ~TelemetryManager();

    void start();
    void stop();
    bool isRunning(){return _running;}

    // Methods to get the latest telemetry data
    Telemetry::Position getLatestPosition();
    Telemetry::Health getLatestHealth();
    Telemetry::Altitude getAltitude();
    Telemetry::EulerAngle getEulerAngle();
    Telemetry::FlightMode getFlightMode();
    Telemetry::Heading getHeading();
    Telemetry::VelocityNed getVelocity();
private:
    void subscribeTelemetry();

    Mavsdk& _mavsdk;
    std::shared_ptr<System> _system;
    std::unique_ptr<Telemetry> _telemetry;

    std::atomic<bool> _running;

    // Containers to store telemetry data
    Telemetry::Position _latest_position;
    Telemetry::Health _latest_health;
    Telemetry::Altitude _latest_altitude;
    Telemetry::EulerAngle _latest_euler_angle;
    Telemetry::FlightMode _latest_flight_mode;
    Telemetry::Heading _latest_heading;
    Telemetry::VelocityNed _latest_velocity;


    std::mutex _data_mutex;
};

#endif // TELEMETRY_MANAGER_H
