#include "UDPServer.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "../../Events/EventManager.h"

UDPServer::UDPServer(int port) : port(port), serverSocket(-1), running(false) {
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    commandManager = nullptr;
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

        addClientAddress(clientAddr); // Ensure this function is thread-safe
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

            std::cout << "Processing command: " << message << std::endl;

            if (commandManager != nullptr && commandManager->IsViable()) {
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
                    } else if (commandManager->is_command_valid(command)) {
                        auto result = commandManager->handle_command(command, params);
                        if (result == CommandManager::Result::Success) {
                            std::cout << "Command " << command << " executed successfully." << std::endl;
                        } else {
                            std::cerr << "Command " << command << " failed." << std::endl;
                        }
                    } else {
                        std::cerr << "Invalid command: " << command << std::endl;
                    }
                } else {
                    std::cerr << "Invalid message format: " << message << std::endl;
                }
            } else {
                std::cerr << "Command manager not set or not viable." << std::endl;
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
        std::istringstream ss(clientAddrStr);
        ss >> clientAddr.sin_addr.s_addr >> clientAddr.sin_port;

        ssize_t bytesSent = sendto(serverSocket, message.c_str(), message.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        if (bytesSent < 0) {
            std::cerr << "Failed to send message to client. Error: " << strerror(errno) << std::endl;
        } else {
            std::cout << "Message sent to client: " << message << std::endl;
        }
    }
    return true;
}

void UDPServer::addClientAddress(const sockaddr_in& clientAddr) {
    std::string addrStr = clientAddrToString(clientAddr);
    std::lock_guard<std::mutex> lock(clientAddressesMutex);
    clientAddresses.insert(addrStr); // Use unordered_set to automatically handle duplicates
}

std::string UDPServer::clientAddrToString(const sockaddr_in& clientAddr) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, addr, sizeof(addr));
    return std::string(addr) + ":" + std::to_string(ntohs(clientAddr.sin_port));
}

void UDPServer::setCommandManager(std::shared_ptr<CommandManager> command) {
    commandManager = command;
}
