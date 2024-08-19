#include "CommandManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#define COPTER_MODE_STABILIZE 0
CommandManager::CommandManager(const std::shared_ptr<mavsdk::System>& system) : system(system)
{
    action = std::make_shared<mavsdk::Action>(system);
    manual_control = std::make_shared<mavsdk::ManualControl>(system);
    mavlink_passthrough = std::make_shared<mavsdk::MavlinkPassthrough>(system);
    viable = system->is_connected();

    system->subscribe_is_connected([this](bool connected) {
        if(!connected){
            viable = false;
        } else {
            viable = true;
        }
    });
    initialize_command_handlers();
}

bool CommandManager::IsViable() { return viable; }

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

bool CommandManager::is_command_valid(const std::string& command) const {
    return command_map.find(command) != command_map.end();
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
    std::cout<<"cjeck"<<std::endl;

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
        return Result::ConnectionError;
    }

    auto result = action_func();
    if (result != mavsdk::Action::Result::Success) {
        std::cerr << action_name << " failed: " << result << std::endl;
        return Result::CommandFailed;
    }
    std::cout << "command successful\n";

    return Result::Success;
}

CommandManager::Result CommandManager::set_manual_control_impl(float x, float y, float z, float r) {
    // Define constant values for the other parameters
    const uint8_t target = system->get_system_id();            // Example target system ID
    const uint16_t buttons = 0;          // Default buttons bitmask
    const uint16_t buttons2 = 0;         // Default secondary buttons bitmask
    const uint8_t enabled_extensions = 0;// No extensions enabled
    const int16_t s = 0;                 // Default additional control input S
    const int16_t t = 0;                 // Default additional control input T
    const int16_t aux1 = 0;              // Default auxiliary control 1
    const int16_t aux2 = 0;              // Default auxiliary control 2
    const int16_t aux3 = 0;              // Default auxiliary control 3
    const int16_t aux4 = 0;              // Default auxiliary control 4
    const int16_t aux5 = 0;              // Default auxiliary control 5
    const int16_t aux6 = 0;              // Default auxiliary control 6
    const int16_t pitch_only_axis = 0;
    const int16_t roll_only_axis = 0;
    auto result = mavlink_passthrough->queue_message([&](MavlinkAddress mavlink_address, uint8_t channel) {
        mavlink_message_t message;
        mavlink_msg_manual_control_pack_chan(
            mavlink_address.system_id,
            mavlink_address.component_id,
            channel,
            &message,
            target,
            static_cast<int16_t>(x * 1000),
            static_cast<int16_t>(y * 1000),
            static_cast<int16_t>(z * 1000),
            static_cast<int16_t>(r * 1000),
            buttons,
            buttons2,
            enabled_extensions,
            pitch_only_axis,
            roll_only_axis,
            0,
            0,
            0,
            0,
            0,
            0);
        return message;
    });

    if (result != mavsdk::MavlinkPassthrough::Result::Success) {
        std::cerr << "Failed to queue manual control command" << std::endl;
        return Result::Failure;
    }
    std::cout<<"check"<< std::endl;
    return Result::Success;
}
CommandManager::Result CommandManager::start_manual_control() {
    // Set the flight mode to MANUAL (or another suitable mode)
    auto mode_result = mavlink_passthrough->queue_message([&](MavlinkAddress mavlink_address, uint8_t channel) {
        mavlink_message_t message;
        mavlink_msg_set_mode_pack_chan(
            mavlink_address.system_id,
            mavlink_address.component_id,
            channel,
            &message,
            system->get_system_id(),
            MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, // Base mode indicating custom mode enabled
            COPTER_MODE_STABILIZE              // Stabilize mode value
        );
        return message;
    });


    if (mode_result != mavsdk::MavlinkPassthrough::Result::Success) {
        std::cerr << "Failed to set manual flight mode" << std::endl;
        return Result::Failure;
    }

    // After setting the manual mode, you can now send manual control commands.
    // Example initial manual control command:
    auto control_result = mavlink_passthrough->queue_message([&](MavlinkAddress mavlink_address, uint8_t channel) {
        mavlink_message_t message;
        mavlink_msg_manual_control_pack_chan(
                mavlink_address.system_id,
                mavlink_address.component_id,
                channel,
                &message,
                system->get_system_id(),             // Target system
                0,                                   // X axis control (neutral position)
                0,                                   // Y axis control (neutral position)
                500,                                 // Z axis control (neutral throttle position)
                0,                                   // R axis control (neutral yaw)
                0,                                   // Buttons bitmask (no buttons pressed)
                0,                                   // Secondary buttons bitmask
                0,                                   // No extensions enabled
                0,                                   // Additional control input S (neutral)
                0,                                   // Additional control input T (neutral)
                0,                                   // Auxiliary control 1 (neutral)
                0,                                   // Auxiliary control 2 (neutral)
                0,                                   // Auxiliary control 3 (neutral)
                0,                                   // Auxiliary control 4 (neutral)
                0,                                   // Auxiliary control 5 (neutral)
                0                                    // Auxiliary control 6 (neutral)
        );
        return message;
    });

    if (control_result != mavsdk::MavlinkPassthrough::Result::Success) {
        std::cerr << "Failed to send manual control command" << std::endl;
        return Result::Failure;
    }
    std::cout<<"succuss starting man"<<std::endl;

    return Result::Success;
}
