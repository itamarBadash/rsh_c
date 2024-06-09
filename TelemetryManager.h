#ifndef TELEMETRY_MANAGER_H
#define TELEMETRY_MANAGER_H

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <iostream>

using namespace mavsdk;

class TelemetryManager {
public:
    TelemetryManager(Mavsdk& mavsdk);
    ~TelemetryManager();

    void start();
    void stop();

    // Methods to get the latest telemetry data
    Telemetry::Position getLatestPosition();
    Telemetry::Health getLatestHealth();

private:
    void telemetryThread();

    Mavsdk& _mavsdk;
    std::shared_ptr<System> _system;
    std::unique_ptr<Telemetry> _telemetry;

    std::thread _thread;
    std::atomic<bool> _running;
    std::mutex _mutex;
    std::condition_variable _cv;

    // Containers to store telemetry data
    Telemetry::Position _latest_position;
    Telemetry::Health _latest_health;

    std::mutex _data_mutex;
};

#endif // TELEMETRY_MANAGER_H
