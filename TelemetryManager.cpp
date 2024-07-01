
#include "TelemetryManager.h"
#include <chrono>
#include <thread>
#include <iostream>

TelemetryManager::TelemetryManager(const std::shared_ptr<System>& system, std::shared_ptr<CommunicationManager> comm)
        : _system(system), communication_manager(comm), _running(false)  {

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
    communication_manager->sendMessage(_latest_telemetry_data.print());
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
    if (!_running) return;
    _running = false;
}

void TelemetryManager::subscribeTelemetry() {
    _telemetry->subscribe_position([this](Telemetry::Position position) {
        if (hasSignificantChange(position, _latest_telemetry_data.position)) {
            _latest_telemetry_data.position = position;
            CommunicationManager::Result result = communication_manager->sendMessage(_latest_telemetry_data.print());
        }
    });

    _telemetry->subscribe_health([this](Telemetry::Health health) {
        if (hasSignificantChange(health, _latest_telemetry_data.health)) {
            _latest_telemetry_data.health = health;
            CommunicationManager::Result result = communication_manager->sendMessage(_latest_telemetry_data.print());
        }
    });

    _telemetry->subscribe_altitude([this](Telemetry::Altitude altitude) {
            _latest_telemetry_data.altitude = altitude;
            CommunicationManager::Result result = communication_manager->sendMessage(_latest_telemetry_data.print());

    });

    _telemetry->subscribe_attitude_euler([this](Telemetry::EulerAngle eulerAngle) {
        if (hasSignificantChange(eulerAngle, _latest_telemetry_data.euler_angle)) {
            _latest_telemetry_data.euler_angle = eulerAngle;
            CommunicationManager::Result result = communication_manager->sendMessage(_latest_telemetry_data.print());
        }
    });

    _telemetry->subscribe_flight_mode([this](Telemetry::FlightMode flightMode) {
        if (hasSignificantChange(flightMode, _latest_telemetry_data.flight_mode)) {
            _latest_telemetry_data.flight_mode = flightMode;
            CommunicationManager::Result result = communication_manager->sendMessage(_latest_telemetry_data.print());
        }
    });

    _telemetry->subscribe_velocity_ned([this](Telemetry::VelocityNed velocityNed) {
        if (hasSignificantChange(velocityNed, _latest_telemetry_data.velocity)) {
            _latest_telemetry_data.velocity = velocityNed;
            CommunicationManager::Result result = communication_manager->sendMessage(_latest_telemetry_data.print());
        }
    });

    _telemetry->subscribe_heading([this](Telemetry::Heading heading) {
        if (hasSignificantChange(heading, _latest_telemetry_data.heading)) {
            _latest_telemetry_data.heading = heading;
            CommunicationManager::Result result = communication_manager->sendMessage(_latest_telemetry_data.print());
        }
    });
}

bool TelemetryManager::hasSignificantChange(const Telemetry::Position& new_data, const Telemetry::Position& old_data) {
    auto isDifferent = [](double a, double b) {
        return std::abs(a - b) > 0.0001;
    };

    return isDifferent(new_data.latitude_deg, old_data.latitude_deg) ||
           isDifferent(new_data.longitude_deg, old_data.longitude_deg) ||
           isDifferent(new_data.relative_altitude_m, old_data.relative_altitude_m) ||
           isDifferent(new_data.absolute_altitude_m, old_data.absolute_altitude_m);
}

bool TelemetryManager::hasSignificantChange(const Telemetry::Health& new_data, const Telemetry::Health& old_data) {
    return new_data.is_gyrometer_calibration_ok != old_data.is_gyrometer_calibration_ok ||
           new_data.is_accelerometer_calibration_ok != old_data.is_accelerometer_calibration_ok ||
           new_data.is_magnetometer_calibration_ok != old_data.is_magnetometer_calibration_ok;
}

bool TelemetryManager::hasSignificantChange(const Telemetry::EulerAngle& new_data, const Telemetry::EulerAngle& old_data) {
    auto isDifferent = [](double a, double b) {
        return std::abs(a - b) > 0.0001;
    };

    return isDifferent(new_data.roll_deg, old_data.roll_deg) ||
           isDifferent(new_data.pitch_deg, old_data.pitch_deg) ||
           isDifferent(new_data.yaw_deg, old_data.yaw_deg);
}

bool TelemetryManager::hasSignificantChange(const Telemetry::FlightMode& new_data, const Telemetry::FlightMode& old_data) {
    return new_data != old_data;
}

bool TelemetryManager::hasSignificantChange(const Telemetry::Heading& new_data, const Telemetry::Heading& old_data) {
    auto isDifferent = [](double a, double b) {
        return std::abs(a - b) > 0.0001;
    };

    return isDifferent(new_data.heading_deg, old_data.heading_deg);
}

bool TelemetryManager::hasSignificantChange(const Telemetry::VelocityNed& new_data, const Telemetry::VelocityNed& old_data) {
    auto isDifferent = [](double a, double b) {
        return std::abs(a - b) > 0.0001;
    };

    return isDifferent(new_data.north_m_s, old_data.north_m_s) ||
           isDifferent(new_data.east_m_s, old_data.east_m_s) ||
           isDifferent(new_data.down_m_s, old_data.down_m_s);
}

Telemetry::Position TelemetryManager::getLatestPosition() {
    return _latest_telemetry_data.position;
}

Telemetry::Health TelemetryManager::getLatestHealth() {
    return _latest_telemetry_data.health;
}

float TelemetryManager::getRelativeAltitude() {
    return _latest_telemetry_data.position.relative_altitude_m;
}

Telemetry::EulerAngle TelemetryManager::getEulerAngle() {
    return _latest_telemetry_data.euler_angle;
}

Telemetry::FlightMode TelemetryManager::getFlightMode() {
    return _latest_telemetry_data.flight_mode;
}

Telemetry::Heading TelemetryManager::getHeading() {
    return _latest_telemetry_data.heading;
}

Telemetry::VelocityNed TelemetryManager::getVelocity() {
    return _latest_telemetry_data.velocity;
}

TelemetryData TelemetryManager::getTelemetryData() {
    return _latest_telemetry_data;
}