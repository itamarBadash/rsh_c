#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <iostream>
#include <string>
#include <regex>
#include <memory>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/system/error_code.hpp>

class CommunicationManager {
public:
    // Enum to represent the result of operations
    enum class Result {
        Success,
        AlreadyConnected,
        NotConnected,
        InvalidPort,
        Error
    };

    // Constructor
    CommunicationManager();

    // Destructor
    ~CommunicationManager();

    // Connect to the specified serial port with the given baud rate
    Result connect(const std::string& port, unsigned int baud_rate);

    // Disconnect from the serial port
    void disconnect();

    // Write data to the serial port
    Result write(const std::string& data);

    // Read data from the serial port
    std::string read();

    // Check if the serial port is connected
    bool isConnected() const;

private:
    // Validate the port name
    bool validatePort(const std::string& port);

    // Handle completion of write operation
    void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

    // Handle completion of read operation
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);

    // Boost.Asio service for asynchronous operations
    boost::asio::io_service io_service_;

    // Unique pointer to the serial port
    std::unique_ptr<boost::asio::serial_port> serial_port_;

    // Flag indicating if the serial port is connected
    bool connected_;

    // Strand to ensure thread-safe operations
    boost::asio::io_service::strand strand_;
};

#endif // COMMUNICATIONMANAGER_H
