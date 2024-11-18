#include "CommunicationManager.h"
#include <iostream>
#include "TelemetryManager.h"
#include "../Communications/UDPServer.h"
#include "../Communications/SerialCommunication.h"
#include "../Communications/TCPServer.h"

CommunicationManager::CommunicationManager(CommunicationType communication_type, int port) {
    INIReader reader("../config.ini");
    if (reader.ParseError() < 0) {
        std::cout << "Can't load 'config.ini'\n";
    }

    // Add communication type to the vector during construction
    add_communication(communication_type, port);
}

CommunicationManager::~CommunicationManager() {
    for (auto& communication : communication_ptrs) {
        communication->stop();
    }
}

void CommunicationManager::add_communication(CommunicationType communication_type, int port) {
    std::shared_ptr<ICommunication> communication_ptr;

    INIReader reader("../config.ini");

    switch (communication_type) {
        case ECT_TCP:
            communication_ptr = std::make_shared<TCPServer>(port);
            break;
        case ECT_UDP:
            communication_ptr = std::make_shared<UDPServer>(port);
            break;
        case ECT_SERIAL:
            communication_ptr = std::make_shared<SerialCommunication>(
                reader.GetString("Connection", "GroundStationSerialPort", "UNKNOWN"),
                reader.GetInteger("Connection", "GroundStationBaudRate", 0));
            break;
        default:
            throw std::invalid_argument("Unsupported communication type");
    }

    communication_ptrs.push_back(communication_ptr);
}

void CommunicationManager::send_message_all(const std::string &message) {
    for (auto& communication : communication_ptrs) {
        communication->send_message(message);
    }
}

void CommunicationManager::send_message_by_index(int index,const std::string &message) {
    if (index < 0 || index >= communication_ptrs.size()) {
        throw std::out_of_range("Invalid communication index");
    }
    communication_ptrs[index]->send_message(message);
}

void CommunicationManager::start() {
    for (auto& communication : communication_ptrs) {
        communication->start();
    }
}

void CommunicationManager::stop() {
    for (auto& communication : communication_ptrs) {
        communication->stop();
    }
}

void CommunicationManager::replace_communication_type(int index, CommunicationType new_communication, int port) {
    if (index < 0 || index >= communication_ptrs.size()) {
        throw std::out_of_range("Invalid communication index");
    }

    communication_ptrs[index]->stop();

    std::shared_ptr<ICommunication> new_communication_ptr;

    INIReader reader("../config.ini");
    switch (new_communication) {
        case ECT_TCP:
            new_communication_ptr = std::make_shared<TCPServer>(port);
            break;
        case ECT_UDP:
            new_communication_ptr = std::make_shared<UDPServer>(port);
            break;
        case ECT_SERIAL:
            new_communication_ptr = std::make_shared<SerialCommunication>(
                reader.GetString("Connection", "GroundStationSerialPort", "UNKNOWN"),
                reader.GetInteger("Connection", "GroundStationBaudRate", 0));
            break;
        default:
            throw std::invalid_argument("Unsupported communication type");
    }

    communication_ptrs[index] = new_communication_ptr;
}
