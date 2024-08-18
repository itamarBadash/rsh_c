#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H


#include "../../inih/cpp/INIReader.h"
#include "../Communications/ICommunication.h"

#include "../../Events/EventManager.h"
#include "CommandManager.h"


enum CommunicationType {
    ECT_TCP,
    ECT_UDP,
    ECT_SERIAL
};

class CommunicationManager {
public:
    CommunicationManager(CommunicationType communication);
    ~CommunicationManager();

    CommunicationType communication_type;

    void send_message(const std::string &message);
    void set_command(std::shared_ptr<CommandManager> command_manager);
    void start();

private:
    std::shared_ptr<ICommunication> communication_ptr;

    INIReader reader;
};

#endif //COMMUNICATIONMANAGER_H