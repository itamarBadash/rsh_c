#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <string>
#include <netinet/in.h>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

class TCPServer {
public:
    TCPServer(int port);
    ~TCPServer();

    bool start();
    void stop();
    void handleClient(int clientSocket);
    bool send_message(const std::string& message);

private:
    int serverSocket;
    std::vector<int> clientSockets;
    int port;
    sockaddr_in serverAddr;
    std::atomic<bool> running;
    std::vector<std::thread> clientThreads;
    std::mutex clientSocketsMutex;

    void setupServerAddress();
    void acceptConnections();
    void cleanupThreads();
};

#endif // TCPSERVER_H
