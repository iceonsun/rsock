//
// Created by System Administrator on 12/27/17.
//

#include <iostream>
#include "../RConfig.h"

int fake_main(int argc, char **argv) {
    char *fakeargs[10] = {0};
    fakeargs[0] = argv[0];
    fakeargs[1] = "-f/Users/robert/workspace/cpp/CLIon/rSock/cmake-build-debug/json_conf.txt";
    RConfig conf;
    conf.Parse(true, 2, reinterpret_cast<const char *const *>(fakeargs));
    std::cout << conf.to_json().dump() << std::endl;
    return 0;
}

int argc_main(int argc, char **argv) {
    RConfig conf;
    conf.Parse(true, argc, reinterpret_cast<const char *const *>(argv));
    std::cout << conf.to_json().dump() << std::endl;
    return 0;
}

int main (int argc, char **argv) {
    return fake_main(argc, argv);
}