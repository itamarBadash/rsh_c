//
// Created by itaba on 18/08/2024.
//

#include "CommunicationManager.h"

#include <iostream>



CommunicationManager::CommunicationManager(CommunicationType communication): communication_type(communication),
                                                                             reader("../../config.ini") {
    if (reader.ParseError() < 0) {
        std::cout << "Can't load 'config.ini'\n";
    }

    switch (communication) {
        case 0:
            tcp_server = std::make_shared<TCPServer>(8080);
            tcp_server->start();
            udp_server = nullptr;
            serial_communication = nullptr;
            break;

        case 1:
            tcp_server = nullptr;
            udp_server = std::make_shared<UDPServer>(8080);
            udp_server->start();
            serial_communication = nullptr;

            break;

        case ECT_SERIAL:
            tcp_server = nullptr;
            udp_server = nullptr;
            serial_communication =std::make_shared<SerialCommunication>(reader.GetString("Connection","GroundStationSerialPort","UNKNOWN"),reader.GetInteger("Connection","GroundStationBaudRate",0));
            break;
        default:
            tcp_server = std::make_shared<TCPServer>(8080);
            tcp_server->start();
            udp_server = nullptr;
            break;
    }
}

CommunicationManager::~CommunicationManager() {
    switch (communication_type) {
        case ECT_TCP:
            tcp_server->stop();
        break;

        case ECT_UDP:
            udp_server->stop();
        break;

        default:
            tcp_server->stop();
    }
}
