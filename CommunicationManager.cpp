#include "CommunicationManager.h"

CommunicationManager::CommunicationManager()
        : io_service_(),
          serial_port_(std::make_unique<boost::asio::serial_port>(io_service_)),
          connected_(false),
          strand_(io_service_) {}

CommunicationManager::~CommunicationManager() {
    disconnect();
}

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
    std::cout << "Connected to port " << port << " with baud rate " << baud_rate << std::endl;
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

    std::string data_with_newline = data + "\n";
    std::cout << "Writing data: " << data_with_newline << std::endl;

    boost::system::error_code ec;
    boost::asio::write(*serial_port_, boost::asio::buffer(data_with_newline), ec);
    if (ec) {
        std::cerr << "Error on write: " << ec.message() << std::endl;
        return Result::Error;
    }

    std::cout << "Successfully wrote data." << std::endl;
    return Result::Success;
}

std::string CommunicationManager::read() {
    if (!connected_) {
        throw std::runtime_error("Not connected");
    }

    boost::asio::streambuf buf;
    boost::system::error_code ec;
    boost::asio::read_until(*serial_port_, buf, '\n', ec);
    if (ec) {
        std::cerr << "Error on read: " << ec.message() << std::endl;
        return "";
    }

    std::istream is(&buf);
    std::string line;
    std::getline(is, line);

    std::cout << "Read data: " << line << std::endl;
    return line;
}

bool CommunicationManager::isConnected() const {
    return connected_;
}

bool CommunicationManager::validatePort(const std::string& port) {
#ifdef _WIN32
    std::regex pattern("^COM[0-9]+$");
#else
    std::regex pattern("^/dev/tty(USB|S)[0-9]+$");
#endif
    return std::regex_match(port, pattern);
}
