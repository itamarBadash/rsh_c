#include "CommandManager.h"
#include "../../Events/EventManager.h"
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

CommandManager::~CommandManager() {
    stop_manual_control();
}

bool CommandManager::IsViable() { return viable; }

void CommandManager::initialize_command_handlers() {
    command_map = {
            {"takeoff", [this](const std::vector<float>&) { return takeoff(); }},
            {"land", [this](const std::vector<float>&) { return land(); }},
            {"return_to_launch", [this](const std::vector<float>&) { return return_to_launch(); }},
            {"hold", [this](const std::vector<float>&) { return hold(); }},
            {"stop_manual_control", [this](const std::vector<float>&) { return stop_manual_control(); }},
            {"set_flight_mode", [this](const std::vector<float>& params) {
                if (params.size() == 2) {
                    return set_flight_mode(static_cast<uint8_t>(params[0]), static_cast<uint32_t>(params[1]));
                }
                return Result::Failure;
            }},
            {"start_manual_control", [this](const std::vector<float>&) { return start_manual_control(); }},
            {"set_manual_control", [this](const std::vector<float>& params) {
                if (params.size() == 4) {
                    std::vector<uint16_t> args = {
                        static_cast<uint16_t>(params[0]),
                        static_cast<uint16_t>(params[1]),
                        static_cast<uint16_t>(params[2]),
                        static_cast<uint16_t>(params[3])
                    };
                    return update_manual_control(args);
                }
                return Result::Failure;
            }},
            {"arm", [this](const std::vector<float>&) { return arm(); }},
            {"disarm", [this](const std::vector<float>&) { return disarm(); }},
             {"tap_to_fly", [this](const std::vector<float>&) { return tap_to_fly(); }},
             {"fly_to", [this](const std::vector<float>& params) { return fly_to(params[0],params[1],params[2]); }}
            };
}

bool CommandManager::is_command_valid(const std::string& command) const {
    return command_map.find(command) != command_map.end();
}

CommandManager::Result CommandManager::takeoff() {
    INVOKE_EVENT("send_ack",std::string("takeoff"));
    action->set_takeoff_altitude(20);
    return execute_action([this]() { return action->takeoff(); }, "Takeoff");
}

CommandManager::Result CommandManager::land() {

    INVOKE_EVENT("send_ack",std::string("land"));
    return execute_action([this]() { return action->land(); }, "Landing");
}

CommandManager::Result CommandManager::return_to_launch() {

    INVOKE_EVENT("send_ack",std::string("RTL"));
    return execute_action([this]() { return action->return_to_launch(); }, "Return to launch");
}

CommandManager::Result CommandManager::hold() {
    return execute_action([this]() { return action->hold(); }, "Hold");
}

CommandManager::Result CommandManager::set_flight_mode(uint8_t base_mode, uint32_t custom_mode) {
    INVOKE_EVENT("send_ack",std::string("FLight Mode"));

    return send_mavlink_command(base_mode, custom_mode);
}

CommandManager::Result CommandManager::disarm() {
    INVOKE_EVENT("send_ack",std::string("Disarm"));
    return execute_action([this]() { return action->disarm(); }, "Disarm");
}

CommandManager::Result CommandManager::manual_control_loop() {
    if (!viable) {
        std::cerr << "System not viable for manual control loop" << std::endl;
        return Result::ConnectionError;
    }

    const std::chrono::milliseconds interval(100);  // 0.1 seconds interval

    while (manual_continue_loop) {
        Result result = send_rc_override(manual_channels);
        if (result != Result::Success) {
            std::cerr << "Failed to send RC override in manual control loop" << std::endl;
            return result;
        }
        std::this_thread::sleep_for(interval);
    }

    return Result::Success;

}

CommandManager::Result CommandManager::start_manual_control() {
    if (!viable) {
        std::cerr << "System not viable for manual control loop" << std::endl;
        return Result::ConnectionError;
    }

    if (manual_control_thread.joinable()) {
        stop_manual_control();
    }

    auto result = action->arm();
    if(result != mavsdk::Action::Result::Success) {
        std::cerr << "failed arm" << std::endl;
        return Result::Failure;
    }

    manual_continue_loop = true;
    set_flight_mode(1,5);

        manual_control_thread = std::thread([this]() {
            manual_control_loop();
        });
    return Result::Success;
}

CommandManager::Result CommandManager::stop_manual_control() {
    manual_continue_loop = false;

    if (manual_control_thread.joinable()) {
        manual_control_thread.join();
    }
    auto result = action->return_to_launch();
    if (result != mavsdk::Action::Result::Success) {
        std::cerr << "Return to launch failed: " << result << std::endl;
        return Result::CommandFailed;
    }
    std::cout << "command successful\n";

    return Result::Success;
}

CommandManager::Result CommandManager::update_manual_control(const std::vector<uint16_t> &channels) {
    if (channels.size() != manual_channels.size()) {
        std::cerr << "Invalid channel size. Expected " << manual_channels.size() << " channels." << std::endl;
        return Result::Failure;
    }
    std::lock_guard<std::mutex> lock(manual_control_mutex);
    manual_channels = channels;

    return Result::Success;
}

CommandManager::Result CommandManager::arm() {
    INVOKE_EVENT("send_ack",std::string ("Arm"));
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

CommandManager::Result CommandManager::tap_to_fly() {

    INVOKE_EVENT("send_ack",std::string ("tap_to_fly"));

    return set_flight_mode(1,4);
}

CommandManager::Result CommandManager::fly_to(float lat, float lon, float alt) {
    INVOKE_EVENT("send_ack",std::string ("fly_to"));

    mavsdk::Action::Result actionResult= action->goto_location((double)lat,(double)lon,alt,0);

    if(actionResult == mavsdk::Action::Result::Success)
      return CommandManager::Result::Success;
    return Result::CommandFailed;
}
