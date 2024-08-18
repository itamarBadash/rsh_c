//
// Created by itaba on 18/08/2024.
//

#include "CommunicationManager.h"

#include <iostream>
#include "TelemetryManager.h"
#include "../Communications/UDPServer.h"
#include "../Communications/SerialCommunication.h"
#include "../Communications/TCPServer.h"



CommunicationManager::CommunicationManager(CommunicationType communication): communication_type(communication){

    INIReader reader("../config.ini");
    if (reader.ParseError() < 0) {
        std::cout << "Can't load 'config.ini'\n";
    }

    switch (communication_type) {
        case ECT_TCP:
            communication_ptr = std::make_shared<TCPServer>(8080);
        break;
        case ECT_UDP:
            communication_ptr = std::make_shared<UDPServer>(8080);
        break;
        case ECT_SERIAL:
            communication_ptr = std::make_shared<SerialCommunication>(reader.GetString("Connection","GroundStationSerialPort","UNKNOWN"),reader.GetInteger("Connection","GroundStationBaudRate",0));
        break;

        default:
            throw std::invalid_argument("Unsupported communication type");
    }
}

CommunicationManager::~CommunicationManager() {
    communication_ptr->stop();
}

void CommunicationManager::send_message(const std::string &message) {
    communication_ptr->send_message(message);
}

void CommunicationManager::set_command(std::shared_ptr<CommandManager> command_manager) {
    communication_ptr->setCommandManager(command_manager);
}

void CommunicationManager::start() {
    communication_ptr->start();
}


