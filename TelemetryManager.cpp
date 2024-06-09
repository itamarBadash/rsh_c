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
}

TelemetryManager::~TelemetryManager() {
    stop();
}
void TelemetryManager::start() {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_running) return;
        _running = true;
    }
    _thread = std::thread(&TelemetryManager::telemetryThread, this);
}

void TelemetryManager::stop() {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_running) return;
        _running = false;
    }
    _cv.notify_all();
    if (_thread.joinable()) {
        _thread.join();
    }
}
void TelemetryManager::telemetryThread() {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_running) {
        lock.unlock();

        // Retrieve telemetry data
        Telemetry::Position position = _telemetry->position();
        Telemetry::Health health = _telemetry->health();

        // Store telemetry data
        {
            std::lock_guard<std::mutex> data_lock(_data_mutex);
            _latest_position = position;
            _latest_health = health;
        }

        // Sleep for a bit
        std::this_thread::sleep_for(std::chrono::seconds(1));

        lock.lock();
    }
}
Telemetry::Position TelemetryManager::getLatestPosition() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_position;
}

Telemetry::Health TelemetryManager::getLatestHealth() {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _latest_health;
}
