//
// Created by System Administrator on 12/27/17.
//

#include "../server/ssock.h"

int main(int argc, char **argv) {
    if (argc > 1) {
        return ssock_main(argc, argv);
    }

    char *fakearg[5];

    fakearg[0] = argv[0];
    fakearg[1] = "--dev=lo0";
    fakearg[2] = "--taddr=127.0.0.1:30010";
    fakearg[3] = "--ludp=127.0.0.1";

    return ssock_main(4, fakearg);
}