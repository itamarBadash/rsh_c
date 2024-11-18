#ifndef SERIALCOMMUNICATION_H
#define SERIALCOMMUNICATION_H

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <termios.h>
#include "ICommunication.h"


#include "../Modules/CommandManager.h"


class SerialCommunication : public ICommunication {
public:
    enum class Result {
        Success,
        Failure,
        ConnectionError,
        Unknown
    };
    // Constructor
    SerialCommunication(const std::string &port, int baud_rate);

    // Destructor
    ~SerialCommunication();

    // Send a message via the serial port
    bool send_message(const std::string &message) override;

    bool start() override;
    void stop() override;

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

};

#endif // SERIALCOMMUNICATION_H
