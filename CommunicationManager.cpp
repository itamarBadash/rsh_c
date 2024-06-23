#include "CommunicationManager.h"

CommunicationManager::CommunicationManager(boost::asio::io_service& io_service, const std::string& port, unsigned int baud_rate)
        : io_service_(io_service),
          serial_port_(io_service, port) {
    serial_port_.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
}

void CommunicationManager::start() {
    read();
}

void CommunicationManager::write(const std::string& data) {
    io_service_.post(boost::bind(&CommunicationManager::do_write, this, data));
}

void CommunicationManager::close() {
    io_service_.post(boost::bind(&CommunicationManager::do_close, this));
}

void CommunicationManager::read() {
    boost::asio::async_read_until(serial_port_, boost::asio::dynamic_buffer(read_msg_), '\n',
                                  boost::bind(&CommunicationManager::handle_read, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
}

void CommunicationManager::handle_read(const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (!ec) {
        std::string message(read_msg_.substr(0, bytes_transferred));
        read_msg_.erase(0, bytes_transferred);
        std::cout << "Received: " << message << std::endl;
        read();
    } else {
        std::cerr << "Error during read: " << ec.message() << std::endl;
        do_close();
    }
}

void CommunicationManager::do_write(const std::string& data) {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(data);
    if (!write_in_progress) {
        write();
    }
}

void CommunicationManager::write() {
    boost::asio::async_write(serial_port_, boost::asio::buffer(write_msgs_.front()),
                             boost::bind(&CommunicationManager::handle_write, this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
}

void CommunicationManager::handle_write(const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
    if (!ec) {
        write_msgs_.pop_front();
        if (!write_msgs_.empty()) {
            write();
        }
    } else {
        std::cerr << "Error during write: " << ec.message() << std::endl;
        do_close();
    }
}

void CommunicationManager::do_close() {
    if (serial_port_.is_open()) {
        serial_port_.close();
    }
}
