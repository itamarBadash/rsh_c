#include "CommunicationManager.h"

CommunicationManager::CommunicationManager()
        : io_service_(),
          serial_port_(std::make_unique<boost::asio::serial_port>(io_service_)),
          connected_(false),
          strand_(boost::asio::make_strand(io_service_)),
          read_complete_(false) {}

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

    auto self(shared_from_this());
    boost::asio::post(strand_, [this, self, data]() {
        boost::asio::async_write(*serial_port_, boost::asio::buffer(data),
                                 boost::asio::bind_executor(strand_,
                                                            std::bind(&CommunicationManager::handle_write, this,
                                                                      std::placeholders::_1,
                                                                      std::placeholders::_2)));
    });
    return Result::Success;
}

void CommunicationManager::read() {
    if (!connected_) {
        throw std::runtime_error("Not connected");
    }

    read_complete_ = false;
    auto self(shared_from_this());
    boost::asio::post(strand_, [this, self]() {
        boost::asio::async_read_until(*serial_port_, buffer_, '\n',
                                      boost::asio::bind_executor(strand_,
                                                                 std::bind(&CommunicationManager::handle_read, this,
                                                                           std::placeholders::_1,
                                                                           std::placeholders::_2)));
    });
}

std::string CommunicationManager::getReadData() const {
    return read_data_;
}

bool CommunicationManager::isReadComplete() const {
    return read_complete_;
}

bool CommunicationManager::isConnected() const {
    return connected_;
}

void CommunicationManager::run() {
    io_service_.run();
}

bool CommunicationManager::validatePort(const std::string& port) {
#ifdef _WIN32
    std::regex pattern("^COM[0-9]+$");
#else
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
    if (error) {
        std::cerr << "Error on read: " << error.message() << std::endl;
        connected_ = false;
    } else {
        std::istream is(&buffer_);
        std::getline(is, read_data_);
        read_complete_ = true;
        std::cout << "Read: " << read_data_ << std::endl;
    }
}
