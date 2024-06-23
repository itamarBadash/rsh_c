#include "CommunicationManager.h"
#include <functional> // For std::bind

CommunicationManager::CommunicationManager(boost::asio::io_service& io_service, const std::string& port, unsigned int baud_rate)
        : io_service_(io_service),
          serial_port_(io_service, port),
          run_read_thread_(false) {
    serial_port_.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
    std::cout << "Serial port opened on " << port << " with baud rate " << baud_rate << std::endl;
}

CommunicationManager::~CommunicationManager() {
    close();
}

void CommunicationManager::start() {
    run_read_thread_ = true;
    read_thread_ = std::thread(&CommunicationManager::read_loop, this);
}

void CommunicationManager::write(const std::string& data) {
    io_service_.post(std::bind(&CommunicationManager::do_write, this, data));
}

void CommunicationManager::close() {
    run_read_thread_ = false;
    if (read_thread_.joinable()) {
        read_thread_.join();
    }
    io_service_.post(std::bind(&CommunicationManager::do_close, this));
}

void CommunicationManager::read_loop() {
    try {
        while (run_read_thread_) {
            char c;
            boost::system::error_code ec;
            size_t n = boost::asio::read(serial_port_, boost::asio::buffer(&c, 1), ec);
            if (ec) {
                std::cerr << "Error during read: " << ec.message() << std::endl;
                break;
            }
            if (n > 0) {
                if (c == '\n') {
                    std::cout << "Received: " << read_msg_ << std::endl;
                    read_msg_.clear();
                } else {
                    read_msg_ += c;
                }
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in read loop: " << e.what() << std::endl;
    }
}

void CommunicationManager::do_write(const std::string& data) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(data);
    if (!write_in_progress) {
        boost::asio::async_write(serial_port_, boost::asio::buffer(write_msgs_.front()),
                                 std::bind(&CommunicationManager::handle_write, this,
                                           std::placeholders::_1,
                                           std::placeholders::_2));
    }
}

void CommunicationManager::handle_write(const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    if (!ec) {
        write_msgs_.pop_front();
        if (!write_msgs_.empty()) {
            boost::asio::async_write(serial_port_, boost::asio::buffer(write_msgs_.front()),
                                     std::bind(&CommunicationManager::handle_write, this,
                                               std::placeholders::_1,
                                               std::placeholders::_2));
        }
    } else {
        std::cerr << "Error during write: " << ec.message() << std::endl;
        do_close();
    }
}

void CommunicationManager::do_close() {
    if (serial_port_.is_open()) {
        serial_port_.close();
        std::cout << "Serial port closed" << std::endl;
    }
}
