#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/manual_control/manual_control.h>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <thread>
#include <atomic>
#include <condition_variable>

class CommandManager {
public:
    enum class Result {
        Success,
        Failure,
        ConnectionError,
        CommandUnsupported,
        CommandDenied,
        CommandFailed,
        Unknown
    };

    CommandManager(const std::shared_ptr<mavsdk::System>& system);
    ~CommandManager();

    // Big commands
    Result takeoff();
    Result land();
    Result return_to_launch();
    Result hold();
    Result set_flight_mode(uint8_t base_mode, uint32_t custom_mode);
    Result arm();
    Result disarm();
    Result set_manual_control(float x, float y, float z, float r);
    Result start_manual_control();
    Result start_hold_mode();

    Result handle_command(const std::string& command, const std::vector<float>& parameters);
    bool IsViable();
    bool is_command_valid(const std::string& command) const;

    // Method to provide input externally (e.g., from communication module)
    void provide_control_input(float x, float y, float z, float r);

private:
    std::shared_ptr<mavsdk::Action> action;
    std::shared_ptr<mavsdk::ManualControl> manual_control;
    std::shared_ptr<mavsdk::MavlinkPassthrough> mavlink_passthrough;
    std::shared_ptr<mavsdk::System> system;

    bool viable;
    std::atomic<bool> keep_running;
    std::thread hold_mode_thread;
    std::condition_variable control_input_cv;
    std::mutex control_input_mutex;
    std::vector<float> current_input{0.0f, 0.0f, 0.5f, 0.0f}; // Default to hover

    Result send_mavlink_command(uint8_t base_mode, uint32_t custom_mode);
    using CommandHandler = std::function<Result(const std::vector<float>&)>;
    std::map<std::string, CommandHandler> command_map;
    void initialize_command_handlers();
    CommandManager::Result execute_action(std::function<mavsdk::Action::Result()> action_func, const std::string& action_name);

    // Main loop for handling hold mode
    void hold_mode_loop();
};

#endif // COMMANDMANAGER_H
