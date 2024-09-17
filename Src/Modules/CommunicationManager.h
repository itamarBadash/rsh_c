#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include "../../inih/cpp/INIReader.h"
#include "../Communications/ICommunication.h"
#include "../../Events/EventManager.h"
#include "CommandManager.h"
#include <vector>

enum CommunicationType {
    ECT_TCP,
    ECT_UDP,
    ECT_SERIAL
};

class CommunicationManager {
public:
    CommunicationManager(CommunicationType communication, int port);
    ~CommunicationManager();

    void send_message(const std::string &message);
    void set_command(std::shared_ptr<CommandManager> command_manager);
    void start();
    void replace_communication_type(int index, CommunicationType new_communication, int port);

private:
    std::vector<std::shared_ptr<ICommunication>> communication_ptrs;

    void add_communication(CommunicationType communication, int port);
};

#endif //COMMUNICATIONMANAGER_H
