#ifndef COMMUNICATION_MANAGER_H
#define COMMUNICATION_MANAGER_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <string>
#include <iostream>
#include <regex>

class CommunicationManager {
public:
    enum class Result {
        Success,
        AlreadyConnected,
        InvalidPort,
        Error,
        NotConnected
    };

    CommunicationManager();
    ~CommunicationManager();

    Result connect(const std::string& port, unsigned int baud_rate);
    void disconnect();
    Result write(const std::string& data);
    std::string read();
    bool isConnected() const;
    void run();

private:
    bool validatePort(const std::string& port);
    void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);

    boost::asio::io_service io_service_;
    std::unique_ptr<boost::asio::serial_port> serial_port_;
    boost::asio::io_service::strand strand_;
    bool connected_;
    boost::asio::streambuf buffer_;
};

#endif // COMMUNICATION_MANAGER_H
