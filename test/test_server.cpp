//
// Created by System Administrator on 12/27/17.
//

#include "../server/ssock.h"

#define DEV "lo0"
#define TADDR "127.0.0.1:30010"
#define LPORTS "20005-20010"

int main(int argc, char **argv) {
    if (argc > 1) {
        return ssock_main(argc, argv);
    }

    char *fakearg[10] = {nullptr};

    fakearg[0] = argv[0];
    fakearg[1] = "--dev=" DEV;
    fakearg[2] = "--taddr=" TADDR;
    fakearg[3] = "--ports=" LPORTS;
    fakearg[4] = "-v";

    return ssock_main(5, fakearg);
}