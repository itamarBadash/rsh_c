#include "TCPServer.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <algorithm>
#include <opencv2/imgcodecs.hpp>

#include "../../Events/EventManager.h"


TCPServer::TCPServer(int port) : port(port), serverSocket(-1), running(false) {
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    commandManager = nullptr;
}

TCPServer::~TCPServer() {
    stop();
    cleanupThreads();
}

void TCPServer::setupServerAddress() {
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
}

bool TCPServer::start() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
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

    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Error listening on socket: " << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }

    running = true;
    std::cout << "Server started on port " << port << std::endl;

    std::thread(&TCPServer::acceptConnections, this).detach();
    commandProcessorThread = std::thread(&TCPServer::processCommands, this);

    return true;
}

void TCPServer::stop() {
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

void TCPServer::acceptConnections() {
    while (running) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            if (running) {
                std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            }
            continue;
        }

        std::cout << "Client connected." << std::endl;
        {
            std::lock_guard<std::mutex> lock(clientSocketsMutex);
            clientSockets.push_back(clientSocket);
        }
        clientThreads.emplace_back(&TCPServer::handleClient, this, clientSocket);
    }
}

void TCPServer::handleClient(int clientSocket) {
    const int bufferSize = 1024;
    char buffer[bufferSize];

    while (running) {
        int bytesReceived = recv(clientSocket, buffer, bufferSize - 1, 0);
        if (bytesReceived < 0) {
            std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
            break;
        } else if (bytesReceived == 0) {
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        buffer[bytesReceived] = '\0';

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            commandQueue.push(buffer);
        }
        queueCondition.notify_one();
    }

    close(clientSocket);
    {
        std::lock_guard<std::mutex> lock(clientSocketsMutex);
        clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
    }
}

void TCPServer::processCommands() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCondition.wait(lock, [this] { return !commandQueue.empty() || !running; });

        while (!commandQueue.empty()) {
            std::string message = commandQueue.front();
            commandQueue.pop();
            lock.unlock();

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
                    }
                    else if (command == "set_brightness") {
                        INVOKE_EVENT("set_brightness");
                    }
                    else if (commandManager->is_command_valid(command)) {
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

void TCPServer::cleanupThreads() {
    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    clientThreads.clear();
}

bool TCPServer::send_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(clientSocketsMutex);
    if (clientSockets.empty()) {
        std::cerr << "No clients connected" << std::endl;
        return false;
    }

    size_t totalBytesSent = 0;
    size_t messageLength = message.size();
    const char* messagePtr = message.c_str();

    for (int clientSocket : clientSockets) {
        totalBytesSent = 0;
        while (totalBytesSent < messageLength) {
            ssize_t bytesSent = send(clientSocket, messagePtr + totalBytesSent, messageLength - totalBytesSent, 0);
            if (bytesSent < 0) {
                std::cerr << "Failed to send message to client. Error: " << strerror(errno) << std::endl;
                break;
            }
            totalBytesSent += bytesSent;
        }
    }

    std::cout << "Message sent to all clients: " << message << std::endl;
    return true;
}

void TCPServer::setCommandManager(std::shared_ptr<CommandManager> command) {
    commandManager = command;
}