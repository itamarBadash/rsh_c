#include "BaseAddon.h"
#include <iostream>

BaseAddon::BaseAddon(std::string new_name):
name(new_name) {

}

void BaseAddon::Activate() {
    std::cout << name << "Addon activated successfully." << std::endl;
}