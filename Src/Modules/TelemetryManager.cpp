#include "TelemetryManager.h"
#include <chrono>
#include <thread>
#include <iostream>


TelemetryManager::TelemetryManager(const std::shared_ptr<System>& system)
        : _system(system), _running(false)  {

    system->subscribe_is_connected([this](bool connected) {
        if(!connected){
            viable = false;
        } else
            viable = true;
    });

    viable = true;
    _telemetry = std::make_unique<Telemetry>(_system);

    _latest_telemetry_data.position = _telemetry->position();
    _latest_telemetry_data.health = _telemetry->health();
    _latest_telemetry_data.altitude = _telemetry->altitude();
    _latest_telemetry_data.euler_angle = _telemetry->attitude_euler();
    _latest_telemetry_data.flight_mode = _telemetry->flight_mode();
    _latest_telemetry_data.heading = _telemetry->heading();
    _latest_telemetry_data.velocity = _telemetry->velocity_ned();
}

TelemetryManager::~TelemetryManager() {
    stop();
}

void TelemetryManager::start() {
    if (_running || !viable) return;
    _running = true;
    subscribeTelemetry();
}

void TelemetryManager::stop() {
    {
        if (!_running) return;
        _running = false;
    }
}

void TelemetryManager::subscribeTelemetry() {
    _telemetry->subscribe_position([this](Telemetry::Position position) {
        _latest_telemetry_data.position = position;
    });

    _telemetry->subscribe_health([this](Telemetry::Health health) {
        _latest_telemetry_data.health = health;
    });

    _telemetry->subscribe_altitude([this](Telemetry::Altitude altitude) {
        _latest_telemetry_data.altitude = altitude;
      });

    _telemetry->subscribe_attitude_euler([this](Telemetry::EulerAngle eulerAngle) {
        _latest_telemetry_data.euler_angle = eulerAngle;

    });

    _telemetry->subscribe_flight_mode([this](Telemetry::FlightMode flightMode) {
        _latest_telemetry_data.flight_mode = flightMode;
    });

    _telemetry->subscribe_velocity_ned([this](Telemetry::VelocityNed velocityNed) {
        _latest_telemetry_data.velocity = velocityNed;
    });

    _telemetry->subscribe_heading([this](Telemetry::Heading heading) {
        _latest_telemetry_data.heading = heading;

    });
}

Telemetry::Position TelemetryManager::getLatestPosition() const {
    return _latest_telemetry_data.position;
}

Telemetry::Health TelemetryManager::getLatestHealth() const {
    return _latest_telemetry_data.health;
}

float TelemetryManager::getRelativeAltitude() const {
    return _latest_telemetry_data.position.relative_altitude_m;
}

Telemetry::EulerAngle TelemetryManager::getEulerAngle() const {
    return _latest_telemetry_data.euler_angle;
}

Telemetry::FlightMode TelemetryManager::getFlightMode() const {
    return _latest_telemetry_data.flight_mode;
}

Telemetry::Heading TelemetryManager::getHeading() const {
    return _latest_telemetry_data.heading;
}

Telemetry::VelocityNed TelemetryManager::getVelocity() const {
    return _latest_telemetry_data.velocity;
}

TelemetryData TelemetryManager::getTelemetryData() const {
    return _latest_telemetry_data;
}
