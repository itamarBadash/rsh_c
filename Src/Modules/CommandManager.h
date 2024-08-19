#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/manual_control/manual_control.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <map>

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
    // Command handler
    Result handle_command(const std::string& command, const std::vector<float>& parameters);
    bool IsViable();
    bool is_command_valid(const std::string& command) const;

private:
    std::shared_ptr<mavsdk::Action> action;
    std::shared_ptr<mavsdk::ManualControl> manual_control;
    std::shared_ptr<mavsdk::MavlinkPassthrough> mavlink_passthrough;
    std::shared_ptr<mavsdk::System> system;

    bool viable;

    Result send_mavlink_command(uint8_t base_mode, uint32_t custom_mode);
    // Helper types for command handlers
    using CommandHandler = std::function<Result(const std::vector<float>&)>;

    // Command handler map
    std::map<std::string, CommandHandler> command_map;

    // Initialize command handlers
    void initialize_command_handlers();
    CommandManager::Result execute_action(std::function<mavsdk::Action::Result()> action_func, const std::string& action_name);
};

#endif // COMMANDMANAGER_H