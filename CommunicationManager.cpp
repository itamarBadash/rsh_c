#include "CommunicationManager.h"

CommunicationManager::CommunicationManager()
        : io_service_(),
          serial_port_(std::make_unique<boost::asio::serial_port>(io_service_)),
          connected_(false) {}

CommunicationManager::Result CommunicationManager::connect(const std::string& port, unsigned int baud_rate) {
    if (connected_) {
        return Result::AlreadyConnected;
    }

    if (!validatePort(port)) {
        return Result::InvalidPort;
    }

    boost::system::error_code ec;
    serial_port_->open(port, ec);
    if (ec) {
        std::cerr << "Error opening serial port: " << ec.message() << std::endl;
        return Result::Error;
    }

    serial_port_->set_option(boost::asio::serial_port_base::baud_rate(baud_rate), ec);
    if (ec) {
        std::cerr << "Error setting baud rate: " << ec.message() << std::endl;
        return Result::Error;
    }

    connected_ = true;
    return Result::Success;
}

void CommunicationManager::disconnect() {
    if (connected_) {
        boost::system::error_code ec;
        serial_port_->close(ec);
        if (ec) {
            std::cerr << "Error closing serial port: " << ec.message() << std::endl;
        }
        connected_ = false;
    }
}

CommunicationManager::Result CommunicationManager::write(const std::string& data) {
    if (!connected_) {
        return Result::NotConnected;
    }
    boost::asio::async_write(*serial_port_, boost::asio::buffer(data),
                             boost::bind(&CommunicationManager::handle_write, this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
    return Result::Success;
}

CommunicationManager::Result CommunicationManager::read() {
    if (!connected_) {
        return Result::NotConnected;
    }
    boost::asio::async_read_until(*serial_port_, buffer_, '\n',
                                  boost::bind(&CommunicationManager::handle_read, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
    return Result::Success;
}

bool CommunicationManager::isConnected() const {
    return connected_;
}

void CommunicationManager::run() {
    io_service_.run();
}

bool CommunicationManager::validatePort(const std::string& port) {
#ifdef _WIN32
    // For Windows, port should be in the form "COMx"
        std::regex pattern("^COM[0-9]+$");
#else
    // For Linux, port should be in the form "/dev/ttyUSBx" or "/dev/ttySx"
    std::regex pattern("^/dev/tty(USB|S)[0-9]+$");
#endif
    return std::regex_match(port, pattern);
}

void CommunicationManager::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error) {
        std::cout << "Successfully wrote " << bytes_transferred << " bytes." << std::endl;
    } else {
        std::cerr << "Error on write: " << error.message() << std::endl;
        connected_ = false;
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
        connected_ = false;
    }
}
