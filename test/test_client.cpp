#include "../client/csock.h"

int main(int argc, char **argv) {
    if (argc > 1) {
        return csock_main(argc, argv);
    }

    char *fakearg[10] = {nullptr};
//    for (int i = 0; i < argc; i++) {
//        memcpy(argv2[i], argv[i], strlen(argv[i]));
//    }

    fakearg[0] = argv[0];
    fakearg[1] = "--dev=lo0";
    fakearg[2] = "--ludp=127.0.0.1:30000";
//    fakearg[2] = "--ludp=127.0.0.1:30000";
    fakearg[3] = "--taddr=127.0.0.1";
    fakearg[4] = "--lcapIp=127.0.0.1";
    fakearg[5] = "--tcapPorts=80,443";
    fakearg[6] = "--type=tcp";
    fakearg[7] = "-v";
//    fakearg[4] = "--"

//    sprintf(fakearg[1], "--dev=en0");
//    sprintf(fakearg[2], "--tudp=127.0.0.1:10009");
    return csock_main(8, fakearg);
}