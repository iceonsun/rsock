#include "../client/csock.h"

#define DEV "en0"
#define TIP "165.227.117.179"
//#define DEV "lo0"
//#define TIP "127.0.0.1"
//#define TPORTS "20005-20006"

int main(int argc, char **argv) {
    if (argc > 1) {
        return csock_main(argc, argv);
    }

    char *fakearg[] = {
            argv[0],
            "--dev=" DEV,
            "--ludp=127.0.0.1:30000",
            "--taddr=" TIP,
    };

    return csock_main(sizeof(fakearg) / sizeof(fakearg[0]), fakearg);
}