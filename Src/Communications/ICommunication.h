#ifndef ICOMMUNICATION_H
#define ICOMMUNICATION_H

#include <string>
#include <memory>

class ICommunication {
public:
    virtual ~ICommunication() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool send_message(const std::string &message) = 0;
};

#endif //ICOMMUNICATION_H
