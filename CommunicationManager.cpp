#include "CommunicationManager.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <cstring>
#include <errno.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

CommunicationManager::CommunicationManager(const std::string &port, int baud_rate)
        : port_name(port), baud_rate(baud_rate), serial_port(-1), stop_flag(false) {
    openPort();
    startWorker();
}

CommunicationManager::~CommunicationManager() {
    stopWorker();
    closePort();
}

void CommunicationManager::openPort() {
    serial_port = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

    if (serial_port < 0) {
        std::cerr << "Error " << errno << " opening " << port_name << ": " << strerror(errno) << std::endl;
        return;
    }

    fcntl(serial_port, F_SETFL, 0);

    termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
        closePort();
        return;
    }

    cfsetospeed(&tty, B57600); // Set output speed
    cfsetispeed(&tty, B57600); // Set input speed

    tty.c_cflag &= ~PARENB; // No parity bit
    tty.c_cflag &= ~CSTOPB; // Only one stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8; // 8 bits per byte
    tty.c_cflag &= ~CRTSCTS; // No flow control
    tty.c_cflag |= CREAD | CLOCAL; // Turn on read and ignore control lines

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcsetattr: " << strerror(errno) << std::endl;
        closePort();
        return;
    }

    std::cout << "Serial port opened successfully." << std::endl;
}

void CommunicationManager::closePort() {
    if (serial_port >= 0) {
        close(serial_port);
        serial_port = -1;
    }
}

void CommunicationManager::startWorker() {
    worker_thread = std::thread(&CommunicationManager::workerFunction, this);
}

void CommunicationManager::stopWorker() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        stop_flag = true;
    }
    cond_var.notify_all();
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

void CommunicationManager::workerFunction() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex);
        cond_var.wait(lock, [this]() { return !message_queue.empty() || stop_flag; });

        if (stop_flag) {
            break;
        }

        std::string message = std::move(message_queue.front());
        message_queue.pop();
        lock.unlock();

        sendMessage(message);

        std::string response = receiveMessage();
        if (!response.empty()) {
            std::lock_guard<std::mutex> response_lock(response_mutex);
            last_response = response;
        }
    }
}

void CommunicationManager::sendMessage(const std::string &message) {
    if (serial_port < 0) {
        std::cerr << "Serial port not opened." << std::endl;
        return;
    }

    int n = write(serial_port, message.c_str(), message.size());
    if (n < 0) {
        std::cerr << "Error writing to serial port: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Message sent: " << message << std::endl;
    }
}

std::string CommunicationManager::receiveMessage() {
    if (serial_port < 0) {
        std::cerr << "Serial port not opened." << std::endl;
        return "";
    }

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int n = read(serial_port, buffer, sizeof(buffer));

    if (n > 0) {
        std::string response(buffer, n);
        std::cout << "Received: " << response << std::endl;
        return response;
    } else {
        return "";
    }
}

void CommunicationManager::write(const std::string &message) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        message_queue.push(message);
    }
    cond_var.notify_all();
}

std::string CommunicationManager::getLatestResponse() {
    std::lock_guard<std::mutex> lock(response_mutex);
    return last_response;
}
