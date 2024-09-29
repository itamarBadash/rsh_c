#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <thread>
#include "Src/Modules/TelemetryManager.h"
#include "Src/Modules/CommandManager.h"
#include "Src/Communications/SerialCommunication.h"
#include "inih/cpp/INIReader.h"
#include "Src/Modules/BaseAddon.h"
#include "Events/EventManager.h"
#include "Src/Communications/TCPServer.h"
#include "Src/Communications/UDPServer.h"
#include "Src/Modules/CommunicationManager.h"
#include <chrono>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include "Src/Modules/UDPVideoStreamer.h"

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;
using namespace mavsdk;
using namespace cv;
using namespace std;
class Listener {
public:
    void onEvent(int value) {
        std::cout << "Listener received event with value: " << value << std::endl;
    }

};

void freeFunctionListener(int value) {
    std::cout << "Free function listener received event with value: " << value << std::endl;
}

void usage(const std::string& bin_name) {
    std::cerr << "Usage: " << bin_name << " <connection_url>\n"
              << "Connection URL format should be:\n"
              << " For TCP: tcp://[server_host][:server_port]\n"
              << " For UDP: udp://[bind_host][:bind_port]\n"
              << " For Serial: serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}

void main_thread_function(std::shared_ptr<System> system,
                          std::shared_ptr<CommandManager> command_manager,
                          std::shared_ptr<TelemetryManager> telemetry_manager,
                          std::shared_ptr<CommunicationManager> communication_manager) {
    telemetry_manager->start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void stream_thread_function() {
    try {
        UDPVideoStreamer streamer(0, "192.168.20.8", 12345);  // Use appropriate IP and port
        streamer.stream();
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
}

int main(int argc, char** argv) {

    if (argc != 2) {
        usage(argv[0]);
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

    CREATE_EVENT("InfoRequest");

    auto command_manager = std::make_shared<CommandManager>(system);
    auto telemetry_manager = std::make_shared<TelemetryManager>(system);
    auto communication_manager = std::make_shared<CommunicationManager>(ECT_UDP,8080);

    communication_manager->set_command(command_manager);

    communication_manager->start();

    std::this_thread::sleep_for(std::chrono::seconds(3));


    SUBSCRIBE_TO_EVENT("InfoRequest", ([telemetry_manager, communication_manager]() {
    communication_manager->send_message_all(telemetry_manager->getTelemetryData().print());
    }));

    auto addon = std::make_shared<BaseAddon>("system");

    std::thread stream_thread(stream_thread_function);
    std::thread main_thread(main_thread_function, system, command_manager, telemetry_manager, communication_manager);


    // Print OpenCV version
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;


    main_thread.join();
    stream_thread.join();

    return 0;
}


