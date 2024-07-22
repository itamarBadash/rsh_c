#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <thread>
#include "Src/Modules/TelemetryManager.h"
#include "Src/Modules/CommandManager.h"
#include "CommunicationManager.h"
#include "inih/cpp/INIReader.h"
#include "Src/Modules/BaseAddon.h"
#include "Events/EventManager.h"
class Listener {
public:
    void onEvent(int value) {
        std::cout << "Listener received event with value: " << value << std::endl;
    }
};

void freeFunctionListener(int value) {
    std::cout << "Free function listener received event with value: " << value << std::endl;
}


void main_thread_function(std::shared_ptr<System> system, std::shared_ptr<CommandManager> command_manager,std::shared_ptr<CommunicationManager> communications_manager, std::shared_ptr<TelemetryManager> telemetry_manager);

using namespace mavsdk;

void usage(const std::string& bin_name) {
    std::cerr << "Usage: " << bin_name << " <connection_url>\n"
              << "Connection URL format should be:\n"
              << " For TCP: tcp://[server_host][:server_port]\n"
              << " For UDP: udp://[bind_host][:bind_port]\n"
              << " For Serial: serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}


int main(int argc, char** argv) {
    /*
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }
    INIReader reader("../config.ini");
    if (reader.ParseError() < 0) {
        std::cout << "Can't load 'config.ini'\n";
        return 1;
    }

    Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
    ConnectionResult connection_result = mavsdk.add_any_connection(argv[1]);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result << '\n';
        return 1;
    }

    // Wait for system to connect
    std::cout << "Waiting for system to connect...\n";
    bool system_discovered = false;
    mavsdk.subscribe_on_new_system([&]() {
        const auto systems = mavsdk.systems();
        if (!systems.empty()) {
            system_discovered = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(3));
    if (!system_discovered) {
        std::cerr << "Timed out waiting for system\n";
        return 1;
    }

    auto systems = mavsdk.systems();
    if (systems.empty()) {
        std::cerr << "No systems found\n";
        return 1;
    }

    auto system = systems.at(0);


    auto command_manager = std::make_shared<CommandManager>(system);
    auto communication_manager = std::make_shared<CommunicationManager>(reader.GetString("Connection","GroundStationSerialPort","UNKNOWN"),reader.GetInteger("Connection","GroundStationBaudRate",0),command_manager);
    auto telemetry_manager = std::make_shared<TelemetryManager>(system,communication_manager);
    */

    EventManager& eventManager = GetEventManager();
    auto addon = std::make_shared<BaseAddon>("system");


    // Create an event for activating addons
    eventManager.createEvent<>("AddonActivate");

    // Create a shared instance of BaseAddon

    // Bind the Activate function of the addon to the event
    eventManager.subscribe<>("AddonActivate", std::bind(&BaseAddon::Activate, addon));

    // Invoke the event to activate the addon
    eventManager.invoke<>("AddonActivate");
    return 0;
}

void main_thread_function(std::shared_ptr<System> system, std::shared_ptr<CommandManager> command_manager,std::shared_ptr<CommunicationManager> communications_manager, std::shared_ptr<TelemetryManager> telemetry_manager){
    telemetry_manager->start();
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(3));
        //telemetry_manager->sendTelemetryData();
        communications_manager->sendMessage(telemetry_manager->getTelemetryData().print());
        //logic for error handling and exeptions or retry connections with modules.
    }
}
