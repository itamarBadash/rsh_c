#include "CommandManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <mavsdk/mavlink/common/mavlink.h>


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

CommandManager::Result CommandManager::start_manual_control() {
    set_manual_control(0.f, 0.f, 0.5f, 0.f);
    auto manual_control_result = manual_control->start_position_control();
    if (manual_control_result != mavsdk::ManualControl::Result::Success) {
        std::cerr << "Position control start failed: " << manual_control_result << '\n';
        return Result::Failure;
    }
    return Result::Success;
}

CommandManager::Result CommandManager::send_rc_override(uint16_t channel1, uint16_t channel2, uint16_t channel3, uint16_t channel4) {
    std::cout<<"A"<<std::endl;
    auto result = mavlink_passthrough->queue_message([&](MavlinkAddress mavlink_address, uint8_t channel) {
        mavlink_message_t message;

        // Create and populate the RC override message
        mavlink_rc_channels_override_t rc_override;
        memset(&rc_override, 0, sizeof(rc_override));

        // Set the provided channels
        rc_override.chan1_raw = channel1;
        rc_override.chan2_raw = channel2;
        rc_override.chan3_raw = channel3;
        rc_override.chan4_raw = channel4;

        // Set the other channels to not override
        rc_override.chan5_raw = UINT16_MAX;  // UINT16_MAX indicates no override for these channels
        rc_override.chan6_raw = UINT16_MAX;
        rc_override.chan7_raw = UINT16_MAX;
        rc_override.chan8_raw = UINT16_MAX;
        rc_override.chan9_raw = UINT16_MAX;
        rc_override.chan10_raw = UINT16_MAX;
        rc_override.chan11_raw = UINT16_MAX;
        rc_override.chan12_raw = UINT16_MAX;
        rc_override.chan13_raw = UINT16_MAX;
        rc_override.chan14_raw = UINT16_MAX;
        rc_override.chan15_raw = UINT16_MAX;
        rc_override.chan16_raw = UINT16_MAX;
        rc_override.chan17_raw = UINT16_MAX;
        rc_override.chan18_raw = UINT16_MAX;

        rc_override.target_system = mavlink_address.system_id;
        rc_override.target_component = mavlink_address.component_id;

        // Pack the message into the MAVLink message structure
        mavlink_msg_rc_channels_override_pack_chan(
            mavlink_address.system_id,
            mavlink_address.component_id,
            channel,
            &message,
            rc_override.target_system,
            rc_override.target_component,
            rc_override.chan1_raw,
            rc_override.chan2_raw,
            rc_override.chan3_raw,
            rc_override.chan4_raw,
            rc_override.chan5_raw,
            rc_override.chan6_raw,
            rc_override.chan7_raw,
            rc_override.chan8_raw,
            rc_override.chan9_raw,
            rc_override.chan10_raw,
            rc_override.chan11_raw,
            rc_override.chan12_raw,
            rc_override.chan13_raw,
            rc_override.chan14_raw,
            rc_override.chan15_raw,
            rc_override.chan16_raw,
            rc_override.chan17_raw,
            rc_override.chan18_raw
        );

        return message;
    });

    if (result != mavsdk::MavlinkPassthrough::Result::Success) {
        std::cerr << "Failed to send RC override message" << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}

