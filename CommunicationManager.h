#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <string>

class CommunicationManager {
public:
    CommunicationManager(const std::string &port, int baud_rate);
    ~CommunicationManager();

    void sendMessage(const std::string &message);
    std::string receiveMessage();
    void run();

private:
    int serial_port;
    std::string port_name;
    int baud_rate;

    void openPort();
    void closePort();
    void sendAcknowledgement();
};

#endif // COMMUNICATIONMANAGER_H
