#include "CommunicationManager.h"

CommunicationManager::CommunicationManager(const std::string& port, unsigned int baud_rate, boost::asio::io_service* io_service)
        : internal_io_service_(io_service ? nullptr : std::make_unique<boost::asio::io_service>()),
          io_service_(io_service ? *io_service : *internal_io_service_),
          serial_port_(io_service_, port) {
    serial_port_.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
}

void CommunicationManager::write(const std::string& data) {
    boost::asio::async_write(serial_port_, boost::asio::buffer(data),
                             boost::bind(&CommunicationManager::handle_write, this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
}

void CommunicationManager::read() {
    boost::asio::async_read_until(serial_port_, buffer_, '\n',
                                  boost::bind(&CommunicationManager::handle_read, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
}

void CommunicationManager::run() {
    io_service_.run();
}

void CommunicationManager::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error) {
        std::cout << "Successfully wrote " << bytes_transferred << " bytes." << std::endl;
    } else {
        std::cerr << "Error on write: " << error.message() << std::endl;
    }
}

void CommunicationManager::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error) {
        std::istream is(&buffer_);
        std::string line;
        std::getline(is, line);
        std::cout << "Read: " << line << std::endl;

        // Continue reading
        read();
    } else {
        std::cerr << "Error on read: " << error.message() << std::endl;
    }
}
