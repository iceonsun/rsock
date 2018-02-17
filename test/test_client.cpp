#include "../client/csock.h"

#define DEV "lo0"
#define TIP "127.0.0.1"
#define TPORTS "20005-20006"

// todo: update test_client, test_server.cpp in remote repo
int main(int argc, char **argv) {
    if (argc > 1) {
        return csock_main(argc, argv);
    }

    char *fakearg[10] = {nullptr};

    fakearg[0] = argv[0];
    fakearg[1] = "--dev=" DEV;
    fakearg[2] = "--ludp=127.0.0.1:30000";
    fakearg[3] = "--taddr=" TIP;
    fakearg[4] = "--ports=" TPORTS;
    fakearg[5] = "-v";

    return csock_main(6, fakearg);
}