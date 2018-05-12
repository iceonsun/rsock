#include <time.h>
#include <cerrno>
#include "os.h"

#include "os_util.h"

int os_init_onstartup() {
    return 0;
}

int os_clean() {
    return 0;
}

void rgettimeofday(struct timeval *val) {
    gettimeofday(val, 0);
}


void CloseSocket(int sock) {
    if (sock >= 0) {
        close(sock);
    }
}

int GetPrevSockErr() {
    return errno;
}
