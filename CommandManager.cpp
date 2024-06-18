#include "CommandManager.h"
#include <iostream>
#include <chrono>
#include <thread>


CommandManager::CommandManager(const std::shared_ptr<mavsdk::System>& system) : system(system)
{
    action = std::make_shared<mavsdk::Action>(system);
    manual_control = std::make_shared<mavsdk::ManualControl>(system);
    mavlink_passthrough = std::make_shared<mavsdk::MavlinkPassthrough>(system);
    viable = true;

    system->subscribe_is_connected([this](bool connected) {
        if(!connected){
            viable = false;
        } else
            viable = true;
    });
    initialize_command_handlers();
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
    };
}

CommandManager::Result CommandManager::takeoff() {
    action->set_takeoff_altitude(20);
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
    auto start = std::chrono::high_resolution_clock::now();

    auto result = manual_control->set_manual_control_input(x, y, z, r);

    if (!viable || result != mavsdk::ManualControl::Result::Success) {
        std::cerr << "Manual control input failed: " << result << std::endl;
        return Result::Failure;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

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
        return Result::Failure;
    }

    auto result = action_func();
    if (result != mavsdk::Action::Result::Success) {
        std::cerr << action_name << " failed: " << result << std::endl;
        return Result::Failure;
    }
    std::cout << "command successful\n";

    return Result::Success;
}

CommandManager::Result CommandManager::move_forward(float duration) {
    float move_duration = duration; // Adjust the duration to match 50 meters based on your system's speed
    constexpr float control_interval = 0.01; // Time interval for each manual control input in seconds
    constexpr float forward_speed = 0.9; // Forward speed setting for manual control, adjust based on your system's scale
    constexpr float throttle = 0.8; // Throttle setting for manual control, adjust based on your system's scale
    constexpr float yaw = 0.0; // Yaw setting for manual control
    const int iterations = static_cast<int>(move_duration / control_interval);

    for (int i = 0; i < iterations; ++i) {
        auto result = set_manual_control(forward_speed, 0.0, throttle, yaw);
        if (result != Result::Success) {
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(control_interval * 1000)));
    }
    return Result::Success;
}
