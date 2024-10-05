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
#include <unordered_set>
#include <opencv2/core/mat.hpp>

#include "../Modules/CommandManager.h"
#include "ICommunication.h"

class UDPServer : public ICommunication{
public:
    UDPServer(int port);
    ~UDPServer();

    bool start() override;
    void stop() override;
    bool send_message(const std::string& message) override;
    void setCommandManager(std::shared_ptr<CommandManager> command) override;

    bool send_frame(const cv::Mat &frame);

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

    std::unordered_set<std::string> clientAddresses;
    std::mutex clientAddressesMutex;

    void setupServerAddress();
    void receiveMessages();
    void processCommands();
    void addClientAddress(const sockaddr_in& clientAddr);
    std::string clientAddrToString(const sockaddr_in& clientAddr);
};

#endif // UDPSERVER_H
