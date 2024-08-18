#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H


#include "../../inih/cpp/INIReader.h"

#include "../Communications/SerialCommunication.h"
#include "../Communications/TCPServer.h"
#include "../Communications/UDPServer.h"


enum CommunicationType {
    ECT_TCP,
    ECT_UDP,
    ECT_SERIAL
};

class CommunicationManager {
public:
    CommunicationManager(CommunicationType communication);
    ~CommunicationManager();

    CommunicationType communication_type;



private:
    std::shared_ptr<TCPServer> tcp_server;
    std::shared_ptr<UDPServer> udp_server;
    std::shared_ptr<SerialCommunication> serial_communication;

    INIReader reader;
};

#endif //COMMUNICATIONMANAGER_H