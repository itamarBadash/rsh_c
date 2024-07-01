#include "TelemetryManager.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>


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
        CommunicationManager::Result result = communication_manager->sendMessage(std::ostringstream oss;
        oss << "Position: "
            << position.latitude_deg << ", "
            << position.longitude_deg << ", "
            << position.relative_altitude_m << ", "
            << position.absolute_altitude_m << "\n";);
    });

    _telemetry->subscribe_health([this](Telemetry::Health health) {
        _latest_telemetry_data.health = health;
        std::ostringstream oss;

        if(communication_manager)
            CommunicationManager::Result result = communication_manager->sendMessage(oss << "Health: "
                                                                                            << "Gyro: " << (health.is_gyrometer_calibration_ok ? "OK" : "Not OK") << ", "
                                                                                            << "Acc: " << (health.is_accelerometer_calibration_ok ? "OK" : "Not OK") << ", "
                                                                                            << "Mag: " << (health.is_magnetometer_calibration_ok ? "OK" : "Not OK") << "\n";);
    });

    _telemetry->subscribe_altitude([this](Telemetry::Altitude altitude) {
        std::ostringstream oss;

        _latest_telemetry_data.altitude = altitude;
        if(communication_manager)
            CommunicationManager::Result result = communication_manager->sendMessage(_latest_telemetry_data.altitude);
    });

    _telemetry->subscribe_attitude_euler([this](Telemetry::EulerAngle eulerAngle) {
        std::ostringstream oss;

        _latest_telemetry_data.euler_angle = eulerAngle;
        if(communication_manager)
            CommunicationManager::Result result = communication_manager->sendMessage(oss << "Euler Angles: "
                                                                                                 << euler_angle.roll_deg << ", "
                                                                                                 << euler_angle.pitch_deg << ", "
                                                                                                 << euler_angle.yaw_deg << "\n";);
    });

    _telemetry->subscribe_flight_mode([this](Telemetry::FlightMode flightMode) {
        std::ostringstream oss;

        _latest_telemetry_data.flight_mode = flightMode;
        if(communication_manager)
            CommunicationManager::Result result = communication_manager->sendMessage(    oss << "Flight Mode: "
                                                                                             << static_cast<int>(flight_mode) <<"\n";);
    });

    _telemetry->subscribe_velocity_ned([this](Telemetry::VelocityNed velocityNed) {
        std::ostringstream oss;

        _latest_telemetry_data.velocity = velocityNed;
        if(communication_manager)
            CommunicationManager::Result result = communication_manager->sendMessage( oss << "Velocity NED: "
                                                                                          << velocity.north_m_s << ", "
                                                                                          << velocity.east_m_s << ", "
                                                                                          << velocity.down_m_s << "\n";);
    });

    _telemetry->subscribe_heading([this](Telemetry::Heading heading) {
        std::ostringstream oss;

        _latest_telemetry_data.heading = heading;
        if(communication_manager)
            CommunicationManager::Result result = communication_manager->sendMessage( oss << "Heading: "
                                                                                          << heading.heading_deg << "\n";);
    });
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
