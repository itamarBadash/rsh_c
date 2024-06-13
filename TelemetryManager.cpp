#include "TelemetryManager.h"
#include <chrono>
#include <thread>
#include <iostream>
// Include this header for std::this_thread
TelemetryManager::TelemetryManager(Mavsdk& mavsdk)
        : _mavsdk(mavsdk), _running(false) {

    std::cout << "Waiting to discover system..." << std::endl;
    bool discovered_system = false;
    _mavsdk.subscribe_on_new_system([this, &discovered_system]() {
        const auto system = _mavsdk.systems().back();

        if (system && system->has_autopilot()) {
            _system = system;
            discovered_system = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(2));
    if (!discovered_system) {
        throw std::runtime_error("No autopilot found");
    }

    _telemetry = std::make_unique<Telemetry>(_system);

    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_telemetry_data.position = _telemetry->position();
        _latest_telemetry_data.health = _telemetry->health();
        _latest_telemetry_data.altitude = _telemetry->altitude();
        _latest_telemetry_data.euler_angle = _telemetry->attitude_euler();
        _latest_telemetry_data.flight_mode = _telemetry->flight_mode();
        _latest_telemetry_data.heading = _telemetry->heading();
        _latest_telemetry_data.velocity = _telemetry->velocity_ned();
    }
}

TelemetryManager::~TelemetryManager() {
    stop();
}

void TelemetryManager::start() {
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        if (_running) return;
        _running = true;
    }
    subscribeTelemetry();
}

void TelemetryManager::stop() {
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        if (!_running) return;
        _running = false;
    }
    _telemetry->subscribe_position(nullptr);
    _telemetry->subscribe_health(nullptr);
    _telemetry->subscribe_altitude(nullptr);
    _telemetry->subscribe_attitude_euler(nullptr);
    _telemetry->subscribe_flight_mode(nullptr);
    _telemetry->subscribe_velocity_ned(nullptr);
    _telemetry->subscribe_heading(nullptr);
}

void TelemetryManager::subscribeTelemetry() {
    _telemetry->subscribe_position([this](Telemetry::Position position) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_telemetry_data.position = position;
    });

    _telemetry->subscribe_health([this](Telemetry::Health health) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_telemetry_data.health = health;
    });

    _telemetry->subscribe_altitude([this](Telemetry::Altitude altitude) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_telemetry_data.altitude = altitude;
    });

    _telemetry->subscribe_attitude_euler([this](Telemetry::EulerAngle eulerAngle) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_telemetry_data.euler_angle = eulerAngle;
    });

    _telemetry->subscribe_flight_mode([this](Telemetry::FlightMode flightMode) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_telemetry_data.flight_mode = flightMode;
    });

    _telemetry->subscribe_velocity_ned([this](Telemetry::VelocityNed velocityNed) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_telemetry_data.velocity = velocityNed;
    });

    _telemetry->subscribe_heading([this](Telemetry::Heading heading) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_telemetry_data.heading = heading;
    });
}

Telemetry::Position TelemetryManager::getLatestPosition() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_telemetry_data.position;
}

Telemetry::Health TelemetryManager::getLatestHealth() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_telemetry_data.health;
}

Telemetry::Altitude TelemetryManager::getAltitude() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_telemetry_data.altitude;
}

Telemetry::EulerAngle TelemetryManager::getEulerAngle() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_telemetry_data.euler_angle;
}

Telemetry::FlightMode TelemetryManager::getFlightMode() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_telemetry_data.flight_mode;
}

Telemetry::Heading TelemetryManager::getHeading() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_telemetry_data.heading;
}

Telemetry::VelocityNed TelemetryManager::getVelocity() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_telemetry_data.velocity;
}
TelemetryData TelemetryManager::getTelemetryData() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_telemetry_data;
}