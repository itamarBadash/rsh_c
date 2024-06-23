#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <memory>
#include <regex>
#include <string>

class CommunicationManager {
public:
    enum class Result {
        Success,
        Error,
        NotConnected,
        AlreadyConnected,
        InvalidPort
    };

    CommunicationManager();
    ~CommunicationManager();

    Result connect(const std::string& port, unsigned int baud_rate);
    void disconnect();
    Result write(const std::string& data);
    void read();
    std::string getReadData() const;
    bool isReadComplete() const;
    bool isConnected() const;
    void run();

private:
    bool validatePort(const std::string& port);
    void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);

    boost::asio::io_context io_service_;
    std::unique_ptr<boost::asio::serial_port> serial_port_;
    bool connected_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::streambuf buffer_;
    std::string read_data_;
    bool read_complete_;
};

#endif // COMMUNICATIONMANAGER_H
