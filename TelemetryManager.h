#ifndef TELEMETRY_MANAGER_H
#define TELEMETRY_MANAGER_H

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <atomic>
#include <mutex>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace mavsdk;
struct TelemetryData {
    Telemetry::Position position;
    Telemetry::Health health;
    Telemetry::Altitude altitude;
    Telemetry::EulerAngle euler_angle;
    Telemetry::FlightMode flight_mode;
    Telemetry::Heading heading;
    Telemetry::VelocityNed velocity;

    std::string serialize() const {
        nlohmann::json j;
        j["position"] = {{"latitude", position.latitude}, {"longitude", position.longitude}};
        j["health"] = {{"status", health.status}};
        j["altitude"] = {{"meters", altitude.meters}};
        j["euler_angle"] = {{"roll", euler_angle.roll}, {"pitch", euler_angle.pitch}, {"yaw", euler_angle.yaw}};
        j["flight_mode"] = {{"mode", flight_mode.mode}};
        j["heading"] = {{"degrees", heading.degrees}};
        j["velocity"] = {{"north", velocity.north}, {"east", velocity.east}, {"down", velocity.down}};
        return j.dump();
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
