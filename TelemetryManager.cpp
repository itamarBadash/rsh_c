#include "TelemetryManager.h"

TelemetryManager::TelemetryManager(Mavsdk& mavsdk)
        : _mavsdk(mavsdk), _running(false) {

    std::cout << "Waiting to discover system..." << std::endl;
    bool discovered_system = false;
    _mavsdk.subscribe_on_new_system([this, &discovered_system]() {
        const auto system = _mavsdk.systems().back();

        if (system->has_autopilot()) {
            _system = system;
            discovered_system = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(2));
    if (!discovered_system) {
        throw std::runtime_error("No autopilot found");
    }

    _telemetry = std::make_unique<Telemetry>(_system);

    // Initialize telemetry data from the MAVSDK system
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_position = _telemetry-> position();
        _latest_health = _telemetry-> health();
        _latest_altitude =_telemetry-> altitude();
        _latest_euler_angle = _telemetry-> attitude_euler();
        _latest_flight_mode = _telemetry-> flight_mode();
        _latest_heading = _telemetry-> heading();
        _latest_velocity = _telemetry-> velocity_ned();
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
        _latest_position = position;
    });

    _telemetry->subscribe_health([this](Telemetry::Health health) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_health = health;
    });

    _telemetry->subscribe_altitude([this](Telemetry::Altitude altitude) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_altitude = altitude;
    });

    _telemetry->subscribe_attitude_euler([this](Telemetry::EulerAngle eulerAngle) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_euler_angle = eulerAngle;
    });

    _telemetry->subscribe_flight_mode([this](Telemetry::FlightMode flightMode) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_flight_mode = flightMode;
    });

    _telemetry->subscribe_velocity_ned([this](Telemetry::VelocityNed velocityNed) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_velocity = velocityNed;
    });

    _telemetry->subscribe_heading([this](Telemetry::Heading heading) {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _latest_heading = heading;
    });

}

Telemetry::Position TelemetryManager::getLatestPosition() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_position;
}

Telemetry::Health TelemetryManager::getLatestHealth() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_health;
}

Telemetry::Altitude TelemetryManager::getAltitude() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_altitude;}

Telemetry::EulerAngle TelemetryManager::getEulerAngle() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_euler_angle;}

Telemetry::Heading TelemetryManager::getHeading() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_heading;}

Telemetry::FlightMode TelemetryManager::getFlightMode() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_flight_mode;}

Telemetry::VelocityNed TelemetryManager::getVelocity() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_velocity;
}

