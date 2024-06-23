#ifndef COMMUNICATION_MANAGER_H
#define COMMUNICATION_MANAGER_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>
#include <regex>

class CommunicationManager {
public:
    enum class Result {
        Success,
        Error,
        NotConnected,
        AlreadyConnected,
        InvalidPort
    };

    // Constructor to initialize the io_service
    CommunicationManager();

    // Function to connect to the serial port
    Result connect(const std::string& port, unsigned int baud_rate);

    // Function to disconnect from the serial port
    void disconnect();

    // Function to write data to the serial port
    Result write(const std::string& data);

    // Function to start reading data from the serial port
    std::string read();

    // Function to check if the serial port is open
    bool isConnected() const;

    // Function to run the io_service
    void run();

private:
    // Handler for write operations
    void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

    // Handler for read operations
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);

    // Function to validate the port string
    static bool validatePort(const std::string& port);

    boost::asio::io_service io_service_;                          // Internal io_service object
    std::unique_ptr<boost::asio::serial_port> serial_port_;        // Serial port object
    boost::asio::streambuf buffer_;                                // Buffer to store read data
    bool connected_;                                               // Connection status
};

#endif // COMMUNICATION_MANAGER_H
