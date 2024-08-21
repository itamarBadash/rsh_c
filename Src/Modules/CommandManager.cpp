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

CommandManager::Result CommandManager::send_rc_override(const std::vector<uint16_t>& channels) {
    auto result = mavlink_passthrough->queue_message([&](MavlinkAddress mavlink_address, uint8_t channel) {
        mavlink_message_t message;

        // Ensure we are using the GCS ID (255) for sending the message
        uint8_t gcs_sys_id = 255;
        uint8_t gcs_comp_id = mavlink_address.component_id; // Typically, the component ID is fine as-is

        // Create an array of 18 channels initialized to UINT16_MAX
        uint16_t channel_values[18];
        std::fill_n(channel_values, 18, UINT16_MAX);

        // Copy the provided channel values into the beginning of the array
        std::copy(channels.begin(), channels.end(), channel_values);

        // Pack the RC channels override message
        mavlink_msg_rc_channels_override_pack_chan(
            gcs_sys_id, // Use the GCS system ID
            gcs_comp_id, // Use the appropriate component ID
            channel,
            &message,
            system->get_system_id(), // Target system ID (usually the drone)
            mavlink_address.component_id, // Target component ID
            channel_values[0], // RC channel 1 (Throttle/Yaw/Roll/Pitch as appropriate)
            channel_values[1], // RC channel 2
            channel_values[2], // RC channel 3
            channel_values[3], // RC channel 4
            channel_values[4], // RC channel 5
            channel_values[5], // RC channel 6
            channel_values[6], // RC channel 7
            channel_values[7], // RC channel 8
            channel_values[8], // RC channel 9
            channel_values[9], // RC channel 10
            channel_values[10], // RC channel 11
            channel_values[11], // RC channel 12
            channel_values[12], // RC channel 13
            channel_values[13], // RC channel 14
            channel_values[14], // RC channel 15
            channel_values[15], // RC channel 16
            channel_values[16], // RC channel 17
            channel_values[17]  // RC channel 18
        );

        return message;
    });

    if (result != mavsdk::MavlinkPassthrough::Result::Success) {
        std::cerr << "Failed to send RC override message" << std::endl;
        return Result::Failure;
    }
    return Result::Success;
}

CommandManager::Result CommandManager::send_manual_control(uint16_t pitch, uint16_t roll, uint16_t throttle, uint16_t yaw) {
    auto result = mavlink_passthrough->queue_message([&](MavlinkAddress mavlink_address, uint8_t channel) {
         mavlink_message_t message;

         // Ensure we are using the GCS ID (255) for sending the message
        uint8_t gcs_sys_id = 255;
        uint8_t gcs_comp_id = mavlink_address.component_id; // Typically, the component ID is fine as-is
    mavlink_msg_manual_control_pack_chan(
             gcs_sys_id,
            gcs_comp_id,
             channel,
             &message,
             system->get_system_id(),         // Target system
             pitch,                               // X axis control (neutral)
             roll,                               // Y axis control (neutral)
             throttle,                             // Z axis control (neutral throttle)
             yaw,                               // R axis control (neutral yaw)
             0,                               // Buttons bitmask (no buttons pressed)
             0,                               // Secondary buttons bitmask
             0,                               // No extensions enabled
             0,                               // Additional control input S (neutral)
             0,                               // Additional control input T (neutral)
             0,                               // Auxiliary control 1 (neutral)
             0,                               // Auxiliary control 2 (neutral)
             0,                               // Auxiliary control 3 (neutral)
             0,                               // Auxiliary control 4 (neutral)
             0,                               // Auxiliary control 5 (neutral)
             0                                // Auxiliary control 6 (neutral)
         );
        return message;

    });


    if (result != mavsdk::MavlinkPassthrough::Result::Success) {
        std::cerr << "Failed to send RC override message" << std::endl;
        return Result::Failure;
    }
    return Result::Success;
}