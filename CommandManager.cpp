#include "CommandManager.h"
#include <iostream>

CommandManager::CommandManager(const std::shared_ptr<mavsdk::System>& system) : system(system)
{
    action = std::make_shared<mavsdk::Action>(system);
    manual_control = std::make_shared<mavsdk::ManualControl>(system);
    mavlink_passthrough = std::make_shared<mavsdk::MavlinkPassthrough>(system);
}

CommandManager::Result CommandManager::takeoff() {
    auto result = action->arm();
    if (result != mavsdk::Action::Result::Success) {
        std::cerr << "Arming failed: " << result << std::endl;
        return Result::Failure;
    }

    result = action->takeoff();
    if (result != mavsdk::Action::Result::Success) {
        std::cerr << "Takeoff failed: " << result << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}

CommandManager::Result CommandManager::land() {
    auto result = action->land();
    if (result != mavsdk::Action::Result::Success) {
        std::cerr << "Landing failed: " << result << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}

CommandManager::Result CommandManager::return_to_launch() {
    auto result = action->return_to_launch();
    if (result != mavsdk::Action::Result::Success) {
        std::cerr << "Return to launch failed: " << result << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}

CommandManager::Result CommandManager::hold() {
    auto result = action->hold();
    if (result != mavsdk::Action::Result::Success) {
        std::cerr << "Hold failed: " << result << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}

CommandManager::Result CommandManager::set_flight_mode(uint8_t base_mode, uint32_t custom_mode) {
    return send_mavlink_command(base_mode, custom_mode);
}

CommandManager::Result CommandManager::start_manual_control() {
    auto result = manual_control->start_position_control();
    if (result != mavsdk::ManualControl::Result::Success) {
        std::cerr << "Starting manual control failed: " << result << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}

CommandManager::Result CommandManager::set_manual_control(float x, float y, float z, float r) {
    auto result = manual_control->set_manual_control_input(x, y, z, r);
    if (result != mavsdk::ManualControl::Result::Success) {
        std::cerr << "Manual control input failed: " << result << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}
CommandManager::Result CommandManager::disarm() {
    auto result = action->disarm();

    if (result != mavsdk::Action::Result::Success) {
        std::cerr << "Disarm failed: " << result << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}
CommandManager::Result CommandManager::arm() {
    auto result = action->arm();

    if (result != mavsdk::Action::Result::Success) {
        std::cerr << "Arm failed: " << result << std::endl;
        return Result::Failure;
    }

    return Result::Success;
}

CommandManager::Result CommandManager::handle_command(const std::string& command, const std::vector<float>& parameters) {
    if (command == "takeoff") {
        return takeoff();
    }
    else if (command == "land") {
        return land();
    }
    else if (command == "return_to_launch") {
        return return_to_launch();
    }
    else if (command == "hold") {
        return hold();
    }
    else if (command == "set_flight_mode") {
        if (parameters.size() == 2) {
            return set_flight_mode(static_cast<uint8_t>(parameters[0]), static_cast<uint32_t>(parameters[1]));
        }
    } else if (command == "start_manual_control") {
        return start_manual_control();
    }
    else if (command == "set_manual_control") {
        if (parameters.size() == 4) {
            return set_manual_control(parameters[0], parameters[1], parameters[2], parameters[3]);
        }
    }
    else {
        std::cerr << "Unknown command: " << command << std::endl;
        return Result::Unknown;
    }
    return Result::Failure;
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