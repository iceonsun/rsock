//
// Created by System Administrator on 1/11/18.
//

#include <cstdlib>
#include <initializer_list>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <cstring>
#include "ProcUtil.h"


int ProcUtil::MakeDaemon() {
    const int pid = fork();
    if (pid == 0) {
        const int n = setsid();
        if (-1 == n) {
            return n;
        }
        umask(0);
        const int nret = chdir("/");
        if (nret == -1) {
            return nret;
        }
        for (auto f: {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO}) {
			close(f);	// since we know it's used in unix. just use close to avoid introduce os_util.h
        }
        int fd = open("/dev/null", O_RDWR);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
    } else if (pid > 0) {   // parent;
    } else {
        assert(0);
    }

    return pid;
}

bool ProcUtil::IsRoot() {
    return geteuid() == 0;
}