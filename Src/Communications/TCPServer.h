#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <string>
#include <netinet/in.h>
#include <thread>
#include <atomic>
#include <vector>

class TCPServer {
public:
    TCPServer(int port);
    ~TCPServer();

    bool start();
    void stop();
    void handleClient(int clientSocket);
    void send_message(std::string& message, int clientSocket)

private:
    int serverSocket;
    int port;
    sockaddr_in serverAddr;
    std::atomic<bool> running;
    std::vector<std::thread> clientThreads;

    void setupServerAddress();
    void acceptConnections();
    void cleanupThreads();
};

#endif // TCPSERVER_H
