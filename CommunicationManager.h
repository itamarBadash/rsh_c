#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>


class CommunicationManager {
public:
    // Constructor to initialize the serial port with an optional external io_service
    CommunicationManager(const std::string& port, unsigned int baud_rate, boost::asio::io_service* io_service = nullptr);

    // Function to write data to the serial port
    void write(const std::string& data);

    // Function to start reading data from the serial port
    void read();

    // Function to run the io_service
    void run();

private:
    // Handler for write operations
    void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

    // Handler for read operations
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);

    std::unique_ptr<boost::asio::io_service> internal_io_service_; // Internal io_service object if no external one is provided
    boost::asio::io_service& io_service_;                          // Reference to the io_service object
    boost::asio::serial_port serial_port_;                         // Serial port object
    boost::asio::streambuf buffer_;                                // Buffer to store read data
};

#endif // COMMUNICATIONMANAGER_H
