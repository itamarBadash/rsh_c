#include "CommunicationManager.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <cstring>

CommunicationManager::CommunicationManager(const std::string &port, int baud_rate)
        : port_name(port), baud_rate(baud_rate), serial_port(-1) {
    openPort();
}

CommunicationManager::~CommunicationManager() {
    closePort();
}

void CommunicationManager::openPort() {
    serial_port = open(port_name.c_str(), O_RDWR);

    if (serial_port < 0) {
        std::cerr << "Error " << errno << " opening " << port_name << ": " << strerror(errno) << std::endl;
    }

    termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
    }

    cfsetospeed(&tty, baud_rate);
    cfsetispeed(&tty, baud_rate);

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

    tty.c_cc[VTIME] = 1;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcsetattr: " << strerror(errno) << std::endl;
    }
}

void CommunicationManager::closePort() {
    close(serial_port);
}

void CommunicationManager::sendMessage(const std::string &message) {
    write(serial_port, message.c_str(), message.size());
    std::cout << "Message sent: " << message << std::endl;
}

std::string CommunicationManager::receiveMessage() {
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    int n = read(serial_port, buffer, sizeof(buffer));

    if (n > 0) {
        std::string response(buffer);
        std::cout << "Received: " << response << std::endl;
        sendAcknowledgement();
        return response;
    } else {
        std::cout << "No response received." << std::endl;
        return "";
    }
}

void CommunicationManager::sendAcknowledgement() {
    std::string ack = "ACK";
    write(serial_port, ack.c_str(), ack.size());
    std::cout << "Acknowledgement sent" << std::endl;
}

void CommunicationManager::run() {
    while (true) {
        sendMessage("Hello");
        receiveMessage();
        sleep(2);
    }
}
