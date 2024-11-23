#include "UDPServer.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/mat.hpp>

#include "../../Events/EventManager.h"

UDPServer::UDPServer(int port) : port(port), serverSocket(-1), running(false) {
    std::memset(&serverAddr, 0, sizeof(serverAddr));
}

UDPServer::~UDPServer() {
    stop();
}

void UDPServer::setupServerAddress() {
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
}

bool UDPServer::start() {
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    setupServerAddress();

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }

    running = true;
    std::cout << "UDP Server started on port " << port << std::endl;
    SUBSCRIBE_TO_EVENT("send_ack", ([this]( const std::string & command) {
        send_message("Ack: " + command);
    }));
    std::thread(&UDPServer::receiveMessages, this).detach();
    commandProcessorThread = std::thread(&UDPServer::processCommands, this);

    return true;
}

void UDPServer::stop() {
    if (running) {
        running = false;
        queueCondition.notify_all();
        close(serverSocket);
        std::cout << "Server stopped." << std::endl;

        if (commandProcessorThread.joinable()) {
            commandProcessorThread.join();
        }
    }
}

void UDPServer::receiveMessages() {
    while (running) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        const int bufferSize = 1024;
        char buffer[bufferSize];

        int bytesReceived = recvfrom(serverSocket, buffer, bufferSize - 1, 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesReceived < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
            }
            if (!running) {
                break; // Exit if not running
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent tight loop on error
            continue;
        }

        buffer[bytesReceived] = '\0';

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            commandQueue.emplace(buffer, clientAddr);
        }
        queueCondition.notify_one();

        addClientAddress(clientAddr);
    }
}

void UDPServer::processCommands() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCondition.wait(lock, [this] { return !commandQueue.empty() || !running; });

        while (!commandQueue.empty()) {
            auto [message, clientAddr] = commandQueue.front();
            commandQueue.pop();
            lock.unlock();

            size_t pos = message.find(':');
            if (pos != std::string::npos) {
                std::string command = message.substr(0, pos);
                std::string params_str = message.substr(pos + 1);
                std::vector<float> params;

                size_t start = 0;
                size_t end;
                while ((end = params_str.find(',', start)) != std::string::npos) {
                    try {
                        params.push_back(std::stof(params_str.substr(start, end - start)));
                    } catch (const std::exception& e) {
                        std::cerr << "Error parsing parameter: " << e.what() << std::endl;
                    }
                    start = end + 1;
                }

                if (start < params_str.length()) {
                    try {
                        params.push_back(std::stof(params_str.substr(start)));
                    } catch (const std::exception& e) {
                        std::cerr << "Error parsing parameter: " << e.what() << std::endl;
                    }
                }

                if (command == "info") {
                    INVOKE_EVENT("InfoRequest");
                } else if (command == "set_brightness") {
                    INVOKE_EVENT("set_brightness");
                }
                else {
                     INVOKE_EVENT("command_received", command, params);
                }
            } else {
                std::cerr << "Invalid message format: " << message << std::endl;
            }
            lock.lock();
        }
    }
}

bool UDPServer::send_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(clientAddressesMutex);
    if (clientAddresses.empty()) {
        std::cerr << "No clients to send the message to" << std::endl;
        return false;
    }

    for (const auto& clientAddrStr : clientAddresses) {
        sockaddr_in clientAddr;
        size_t colonPos = clientAddrStr.find(':');
        if (colonPos == std::string::npos) {
            std::cerr << "Invalid client address format: " << clientAddrStr << std::endl;
            continue;
        }

        std::string ip = clientAddrStr.substr(0, colonPos);
        int port = std::stoi(clientAddrStr.substr(colonPos + 1));

        clientAddr.sin_family = AF_INET;
        clientAddr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &clientAddr.sin_addr) <= 0) {
            std::cerr << "Invalid IP address: " << ip << std::endl;
            continue;
        }

        ssize_t bytesSent = sendto(serverSocket, message.c_str(), message.size(), 0,
                                   (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        if (bytesSent < 0) {
            std::cerr << "Failed to send message to client. Error: " << strerror(errno) << std::endl;
        }
    }
    return true;
}

void UDPServer::addClientAddress(const sockaddr_in& clientAddr) {
    std::string addrStr = clientAddrToString(clientAddr);
    std::lock_guard<std::mutex> lock(clientAddressesMutex);
    clientAddresses.insert(addrStr); // unordered_set ensures no duplicates
}

std::string UDPServer::clientAddrToString(const sockaddr_in& clientAddr) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, addr, sizeof(addr));
    return std::string(addr) + ":" + std::to_string(ntohs(clientAddr.sin_port));
}