#ifndef BASE_COMMUNICATIONMANAGER_H
#define BASE_COMMUNICATIONMANAGER_H

#include <iostream>
#include <boost/asio.hpp>

class CommunicationManager {
public:
    CommunicationManager(const std::string& host, const std::string& port);

    void send(const std::string& message);

    std::string receive();

private:
    boost::asio::io_service io_service_;
    boost::asio::ip::tcp::socket socket_;
};

#endif //BASE_COMMUNICATIONMANAGER_H