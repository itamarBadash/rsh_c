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
#include <opencv2/core/mat.hpp>

#include "../Modules/CommandManager.h"
#include "ICommunication.h"

class TCPServer : public ICommunication {
public:
    TCPServer(int port);
    ~TCPServer();

    bool start() override;
    void stop() override;
    void handleClient(int clientSocket);
    bool send_message(const std::string& message) override;
    void setCommandManager(std::shared_ptr<CommandManager> command);

    bool send_frame(const cv::Mat& frame);


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

    void setupServerAddress();
    void acceptConnections();
    void cleanupThreads();
    void processCommands();
};

#endif // TCPSERVER_H
