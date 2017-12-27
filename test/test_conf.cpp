//
// Created by System Administrator on 12/27/17.
//

#include <iostream>
#include "../RConfig.h"

int main (int argc, char **argv) {
    RConfig conf;
    conf.Parse(true, argc, reinterpret_cast<const char *const *>(argv));
    std::cout << conf.to_json().dump() << std::endl;
    return 0;
}