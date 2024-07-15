#include "CommunicationManager.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <cstring>
#include <errno.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <unordered_map>
#include <stdexcept>

CommunicationManager::CommunicationManager(const std::string &port, int baud_rate, std::shared_ptr<CommandManager> cmd_manager)
        : port_name(port), baud_rate(baud_rate), serial_port(-1), stop_flag(false), command_manager(cmd_manager) {
    openPort();
    startWorker();
}

CommunicationManager::~CommunicationManager() {
    stopWorker();
    closePort();
}

speed_t convertBaudRate(int baudRate) {
    switch (baudRate) {
        case 0: return B0;
        case 50: return B50;
        case 75: return B75;
        case 110: return B110;
        case 134: return B134;
        case 150: return B150;
        case 200: return B200;
        case 300: return B300;
        case 600: return B600;
        case 1200: return B1200;
        case 1800: return B1800;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 500000: return B500000;
        case 576000: return B576000;
        case 921600: return B921600;
        case 1000000: return B1000000;
        case 1152000: return B1152000;
        case 1500000: return B1500000;
        case 2000000: return B2000000;
        case 2500000: return B2500000;
        case 3000000: return B3000000;
        case 3500000: return B3500000;
        case 4000000: return B4000000;
        default:
            throw std::invalid_argument("Unsupported baud rate: " + std::to_string(baudRate));
    }
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

    cfsetospeed(&tty, convertBaudRate(baud_rate)); // Set output speed
    cfsetispeed(&tty, convertBaudRate(baud_rate)); // Set input speed

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
    stop_flag = true;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

void CommunicationManager::workerFunction() {
    while (!stop_flag) {
        std::string message = receiveMessage();
        if (!message.empty()) {
            processReceivedMessage(message);
        }
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
        return std::string(buffer, n);
    } else {
        return "";
    }
}

void CommunicationManager::processReceivedMessage(const std::string &message) {

    // Assuming the message is in the format "command:param1,param2,..."
    size_t pos = message.find(':');
    if (pos != std::string::npos) {
        std::string command = message.substr(0, pos);
        std::string params_str = message.substr(pos + 1);
        std::vector<float> params;
        size_t start = 0;
        size_t end;
        while ((end = params_str.find(',', start)) != std::string::npos) {
            params.push_back(std::stof(params_str.substr(start, end - start)));
            start = end + 1;
        }
        if (start < params_str.length()) {
            params.push_back(std::stof(params_str.substr(start)));
        }

        auto result = command_manager->handle_command(command, params);
        if (result == CommandManager::Result::Success) {
            std::cout << "Command " << command << " executed successfully." << std::endl;
        } else {
            std::cerr << "Command " << command << " failed." << std::endl;
        }
    }
}

CommunicationManager::Result CommunicationManager::sendMessage(const std::string &message) {
    std::lock_guard<std::mutex> lock(send_mutex);

    if (serial_port < 0) {
        std::cerr << "Serial port not opened." << std::endl;
        return Result::ConnectionError;
    }

    int n = write(serial_port, message.c_str(), message.size());
    if (n < 0) {
        std::cerr << "Error writing to serial port: " << strerror(errno) << std::endl;
        return Result::Failure;
    } else {
        std::cout << "Message sent: " << message << std::endl;
        return Result::Success;
    }
}