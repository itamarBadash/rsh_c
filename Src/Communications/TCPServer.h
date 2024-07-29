#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <string>
#include <netinet/in.h>

class TCPServer {
public:
    TCPServer(int port);
    ~TCPServer();

    bool start();
    void stop();
    void handleClient(int clientSocket);

private:
    int serverSocket;
    int port;
    sockaddr_in serverAddr;
    bool running;

    void setupServerAddress();
    void acceptConnections();
};

#endif // TCPSERVER_H