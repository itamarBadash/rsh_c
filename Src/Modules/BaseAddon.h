//
// Created by Badash on 22/07/2024.
//

#ifndef BASE_BASEADDON_H
#define BASE_BASEADDON_H

#include <string>


class BaseAddon {
public:

    enum class Result {
        Success,
        Failure,
        ConnectionError,
        CommandUnsupported,
        CommandDenied,
        CommandFailed,
        Unknown
    };

    BaseAddon(std::string new_name);

    std::string name;

    virtual void Activate();

};


#endif //BASE_BASEADDON_H