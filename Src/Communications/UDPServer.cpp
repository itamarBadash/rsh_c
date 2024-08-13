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
        close(serverSocket);
        std::cout << "Server stopped." << std::endl;
        queueCondition.notify_all();
        if (commandProcessorThread.joinable()) {
            commandProcessorThread.join();
        }
    }
}

void UDPServer::receiveMessages() {
    while (running) {
        std::cout << "check"<<std::endl;

        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        const int bufferSize = 1024;
        char buffer[bufferSize];

        int bytesReceived = recvfrom(serverSocket, buffer, bufferSize - 1, 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesReceived < 0) {
            if (running) {
                std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
            }
            continue;
        }

        buffer[bytesReceived] = '\0';

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            commandQueue.emplace(buffer, clientAddr);
        }
        queueCondition.notify_one();

        // Add the client address to the list of known clients
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

            // Process command here
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
                        params.push_back(std::stof(params_str.substr(start, end - start)));
                        start = end + 1;
                    }
                    if (start < params_str.length()) {
                        params.push_back(std::stof(params_str.substr(start)));
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

    for (const auto& clientAddr : clientAddresses) {
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
    std::lock_guard<std::mutex> lock(clientAddressesMutex);
    for (const auto& addr : clientAddresses) {
        if (addr.sin_addr.s_addr == clientAddr.sin_addr.s_addr && addr.sin_port == clientAddr.sin_port) {
            return; // Address already in the list
        }
    }
    clientAddresses.push_back(clientAddr);
}

void UDPServer::setCommandManager(std::shared_ptr<CommandManager> command) {
    commandManager = command;
}
