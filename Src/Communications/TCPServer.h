#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <string>
#include <netinet/in.h>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>
#include "../Src/Modules/CommandManager.h"

class TCPServer {
public:
    TCPServer(int port);
    ~TCPServer();

    bool start();
    void stop();
    void handleClient(int clientSocket);
    bool send_message(const std::string& message);
    void setCommandManager(std::shared_ptr<CommandManager> command);

private:
    int serverSocket;
    std::vector<int> clientSockets;
    int port;
    sockaddr_in serverAddr;
    std::atomic<bool> running;
    std::vector<std::thread> clientThreads;
    std::mutex clientSocketsMutex;

    std::queue<std::string> commandQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::thread commandProcessorThread;
    std::shared_ptr<CommandManager> commandManager;

    void setupServerAddress();
    void acceptConnections();
    void cleanupThreads();
    void processCommands();
};

#endif // TCPSERVER_H
