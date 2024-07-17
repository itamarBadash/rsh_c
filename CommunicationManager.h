#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <termios.h>

#include "Src/Modules/CommandManager.h"


class CommunicationManager {
public:
    enum class Result {
        Success,
        Failure,
        ConnectionError,
        Unknown
    };
    // Constructor
    CommunicationManager(const std::string &port, int baud_rate, std::shared_ptr<CommandManager> cmd_manager);

    // Destructor
    ~CommunicationManager();

    // Send a message via the serial port
    Result sendMessage(const std::string &message);

    speed_t convertBaudRate(int baudRate);

private:
    // Serial port configuration and management
    void openPort();
    void closePort();

    // Asynchronous communication handling
    void startWorker();
    void stopWorker();
    void workerFunction();

    // Message receiving and processing
    std::string receiveMessage();
    void processReceivedMessage(const std::string &message);

    std::string port_name;
    int baud_rate;
    int serial_port;

    std::thread worker_thread;
    std::atomic<bool> stop_flag;

    std::mutex send_mutex;

    std::shared_ptr<CommandManager> command_manager;
};

#endif // COMMUNICATIONMANAGER_H
