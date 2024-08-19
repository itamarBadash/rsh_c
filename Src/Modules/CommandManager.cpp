#include "CommandManager.h"
#include <iostream>
#include <chrono>
#include <thread>

CommandManager::CommandManager(const std::shared_ptr<mavsdk::System>& system)
    : system(system), keep_running(false)
{
    action = std::make_shared<mavsdk::Action>(system);
    manual_control = std::make_shared<mavsdk::ManualControl>(system);
    mavlink_passthrough = std::make_shared<mavsdk::MavlinkPassthrough>(system);
    viable = system->is_connected();

    system->subscribe_is_connected([this](bool connected) {
        viable = connected;
    });

    initialize_command_handlers();
}

CommandManager::~CommandManager() {
    keep_running.store(false);
    if (hold_mode_thread.joinable()) {
        hold_mode_thread.join();
    }
}

bool CommandManager::IsViable() {
    return viable;
}

void CommandManager::initialize_command_handlers() {
    command_map = {
        {"takeoff", [this](const std::vector<float>&) { return takeoff(); }},
        {"land", [this](const std::vector<float>&) { return land(); }},
        {"return_to_launch", [this](const std::vector<float>&) { return return_to_launch(); }},
        {"hold", [this](const std::vector<float>&) { return hold(); }},
        {"set_flight_mode", [this](const std::vector<float>& params) {
            if (params.size() == 2) {
                return set_flight_mode(static_cast<uint8_t>(params[0]), static_cast<uint32_t>(params[1]));
            }
            return Result::Failure;
        }},
        {"set_manual_control", [this](const std::vector<float>& params) {
            if (params.size() == 4) {
                return set_manual_control(params[0], params[1], params[2], params[3]);
            }
            return Result::Failure;
        }},
        {"arm", [this](const std::vector<float>&) { return arm(); }},
        {"disarm", [this](const std::vector<float>&) { return disarm(); }},
        {"start_hold_mode", [this](const std::vector<float>&) { return start_hold_mode(); }}
    };
}

bool CommandManager::is_command_valid(const std::string& command) const {
    return command_map.find(command) != command_map.end();
}

CommandManager::Result CommandManager::takeoff() {
    action->set_takeoff_altitude(5);
    return execute_action([this]() { return action->takeoff(); }, "Takeoff");
}

CommandManager::Result CommandManager::land() {
    return execute_action([this]() { return action->land(); }, "Landing");
}

CommandManager::Result CommandManager::return_to_launch() {
    return execute_action([this]() { return action->return_to_launch(); }, "Return to launch");
}

CommandManager::Result CommandManager::hold() {
    return execute_action([this]() { return action->hold(); }, "Hold");
}

CommandManager::Result CommandManager::set_flight_mode(uint8_t base_mode, uint32_t custom_mode) {
    return send_mavlink_command(base_mode, custom_mode);
}

CommandManager::Result CommandManager::disarm() {
    return execute_action([this]() { return action->disarm(); }, "Disarm");
}

CommandManager::Result CommandManager::arm() {
    return execute_action([this]() { return action->arm(); }, "Arm");
}

CommandManager::Result CommandManager::set_manual_control(float x, float y, float z, float r) {
    if (!viable) {
        std::cerr << "System not viable for manual control input" << std::endl;
        return Result::ConnectionError;
    }

    auto result = manual_control->set_manual_control_input(x, y, z, r);

    if (result != mavsdk::ManualControl::Result::Success) {
        std::cerr << "Manual control input failed with result: " << result << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}

CommandManager::Result CommandManager::handle_command(const std::string& command, const std::vector<float>& parameters) {
    auto it = command_map.find(command);
    if (it != command_map.end()) {
        if (viable) {
            return it->second(parameters);
        } else {
            std::cerr << "System not viable for command: " << command << std::endl;
            return Result::Failure;
        }
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return Result::Unknown;
    }
}

CommandManager::Result CommandManager::send_mavlink_command(uint8_t base_mode, uint32_t custom_mode) {
    auto result = mavlink_passthrough->queue_message([&](MavlinkAddress mavlink_address, uint8_t channel) {
        mavlink_message_t message;
        mavlink_msg_set_mode_pack_chan(
            mavlink_address.system_id,
            mavlink_address.component_id,
            channel,
            &message,
            system->get_system_id(),
            base_mode,
            custom_mode
        );
        return message;
    });

    if (result != mavsdk::MavlinkPassthrough::Result::Success) {
        std::cerr << "Failed to queue MAVLink command" << std::endl;
        return Result::Failure;
    }
    return Result::Success;
}

CommandManager::Result CommandManager::execute_action(std::function<mavsdk::Action::Result()> action_func, const std::string& action_name) {
    if (!viable) {
        std::cerr << action_name << " failed: System not viable" << std::endl;
        return Result::ConnectionError;
    }

    auto result = action_func();
    if (result != mavsdk::Action::Result::Success) {
        std::cerr << action_name << " failed: " << result << std::endl;
        return Result::CommandFailed;
    }
    std::cout << action_name << " successful\n";

    return Result::Success;
}

CommandManager::Result CommandManager::start_manual_control() {
    if (!viable) {
        std::cerr << "System not viable to start manual control" << std::endl;
        return Result::ConnectionError;
    }

    set_manual_control(0.0f, 0.0f, 0.5f, 0.0f);

    // Start position control mode
    auto manual_control_result = manual_control->start_position_control();
    if (manual_control_result != mavsdk::ManualControl::Result::Success) {
        std::cerr << "Failed to start position control: " << manual_control_result << std::endl;
        return Result::Failure;
    }

    std::cout << "Manual control started successfully" << std::endl;
    return Result::Success;
}


CommandManager::Result CommandManager::start_hold_mode() {
    if (!keep_running.load()) {
        keep_running.store(true);
        hold_mode_thread = std::thread(&CommandManager::hold_mode_loop, this);
    }
    return Result::Success;
}

void CommandManager::provide_control_input(float x, float y, float z, float r) {
    {
        std::lock_guard<std::mutex> lock(control_input_mutex);
        current_input = {x, y, z, r};
    }
    control_input_cv.notify_one();
}

void CommandManager::hold_mode_loop() {
    set_manual_control(0.f, 0.f, 0.5f, 0.f); // Default hover command

    while (keep_running.load()) {
        std::unique_lock<std::mutex> lock(control_input_mutex);
        control_input_cv.wait(lock, [this] { return !keep_running.load() || current_input != std::vector<float>{0.0f, 0.0f, 0.5f, 0.0f}; });

        if (!keep_running.load()) {
            break; // Exit loop if not running
        }

        auto result = set_manual_control(current_input[0], current_input[1], current_input[2], current_input[3]);
        if (result != Result::Success) {
            std::cerr << "Error setting manual control input\n";
        }

        // Reset input to hover after applying
        current_input = {0.0f, 0.0f, 0.5f, 0.0f};
    }
}
