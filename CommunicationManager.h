#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

class CommunicationManager {
public:
    CommunicationManager(const std::string &port, int baud_rate);
    ~CommunicationManager();

    // Write a message to the serial port
    void write(const std::string &message);

    // Get the latest response from the serial port
    std::string getLatestResponse();

private:
    // Serial port configuration and management
    void openPort();
    void closePort();

    // Asynchronous communication handling
    void startWorker();
    void stopWorker();
    void workerFunction();

    // Message sending and receiving
    void sendMessage(const std::string &message);
    std::string receiveMessage();

    std::string port_name;
    int baud_rate;
    int serial_port;

    std::thread worker_thread;
    bool stop_flag;

    std::mutex mutex;
    std::condition_variable cond_var;
    std::queue<std::string> message_queue;

    std::mutex response_mutex;
    std::string last_response;
};

#endif // COMMUNICATIONMANAGER_H
