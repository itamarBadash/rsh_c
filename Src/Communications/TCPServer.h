#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <string>
#include <netinet/in.h>
#include <thread>
#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

class TCPServer {
public:
    TCPServer(int port);
    ~TCPServer();

    bool start();
    void stop();
    bool send_message(const std::string& message);

private:
    struct Request {
        int clientSocket;
        std::string command;
    };

    int serverSocket;
    int clientSocket;

    int port;
    sockaddr_in serverAddr;
    std::atomic<bool> running;
    std::vector<std::thread> clientThreads;

    std::queue<Request> requestQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;

    void setupServerAddress();
    void acceptConnections();
    void handleClient(int clientSocket);
    void cleanupThreads();
    void processRequests();
};

#endif // TCPSERVER_H
