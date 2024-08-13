#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <string>
#include <netinet/in.h>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>
#include <unordered_map>
#include <unordered_set>
#include "../Modules/CommandManager.h"

class UDPServer {
public:
    UDPServer(int port);
    ~UDPServer();

    bool start();
    void stop();
    bool send_message(const std::string& message);
    void setCommandManager(std::shared_ptr<CommandManager> command);

private:
    int serverSocket;
    int port;
    sockaddr_in serverAddr;
    std::atomic<bool> running;
    std::thread commandProcessorThread;
    std::shared_ptr<CommandManager> commandManager;

    std::queue<std::pair<std::string, sockaddr_in>> commandQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;

    std::unordered_set<std::string> clientAddresses; // Store as string for simplicity
    std::mutex clientAddressesMutex;

    void setupServerAddress();
    void receiveMessages();
    void processCommands();
    void addClientAddress(const sockaddr_in& clientAddr);
    std::string clientAddrToString(const sockaddr_in& clientAddr);
};

#endif // UDPSERVER_H
