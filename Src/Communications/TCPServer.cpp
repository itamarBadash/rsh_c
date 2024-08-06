#include "TCPServer.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

TCPServer::TCPServer(int port) : port(port), serverSocket(-1), running(false) {
    std::memset(&serverAddr, 0, sizeof(serverAddr));
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

    return true;
}

void TCPServer::stop() {
    if (running) {
        running = false;
        close(serverSocket);
        std::cout << "Server stopped." << std::endl;
    }
}

void TCPServer::acceptConnections() {
    while (running) {
        clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            if (running) {
                std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            }
            continue;
        }

        std::cout << "Client connected." << std::endl;
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

        std::string response = "Message received: ";
        response += buffer;
        response += "\n";
        send(clientSocket, response.c_str(), response.length(), 0);
    }

    close(clientSocket);
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
    if (clientSocket <= 0) {
        std::cerr << "No client connected" << std::endl;
        return false;
    }

    size_t totalBytesSent = 0;
    size_t messageLength = message.size();
    const char* messagePtr = message.c_str();

    while (totalBytesSent < messageLength && clientSocket > 0) {
        ssize_t bytesSent = send(clientSocket, messagePtr + totalBytesSent, messageLength - totalBytesSent, 0);
        if (bytesSent < 0) {
            std::cerr << "Failed to send message. Error: " << strerror(errno) << std::endl;
            return false;
        }
        totalBytesSent += bytesSent;
    }

    std::cout << "Message sent: " << message << std::endl;
    return true;
}
