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

    char *fakearg[] = {
            argv[0],
            "--dev=" DEV,
            "--taddr=" TADDR,
            "--ports=" LPORTS,
            "-v",
    };

    return ssock_main(sizeof(fakearg)/ sizeof(fakearg[0]), fakearg);
}