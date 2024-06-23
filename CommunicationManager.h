#ifndef COMMUNICATION_MANAGER_H
#define COMMUNICATION_MANAGER_H

#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <deque>
#include <string>
#include <atomic>

class CommunicationManager {
public:
    CommunicationManager(boost::asio::io_service& io_service, const std::string& port, unsigned int baud_rate);
    ~CommunicationManager();

    void start();
    void write(const std::string& data);
    void close();

private:
    void read_loop();
    void do_write(const std::string& data);
    void handle_write(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void do_close();

    boost::asio::io_service& io_service_;
    boost::asio::serial_port serial_port_;
    std::deque<std::string> write_msgs_;
    std::string read_msg_;
    std::thread read_thread_;
    std::atomic<bool> run_read_thread_;
    std::mutex write_mutex_;
};

#endif // COMMUNICATION_MANAGER_H
