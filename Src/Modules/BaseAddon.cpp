#include "BaseAddon.h"

BaseAddon::BaseAddon(std::string new_name):
name(new_name) {

}

Result BaseAddon::Activate() {
    std::cout << name << "Addon activated successfully." << std::endl;
    return Result::Success;
}