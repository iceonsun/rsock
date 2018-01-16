//
// Created by System Administrator on 1/11/18.
//

#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ProcUtil.h"

int ProcUtil::MakeDaemon(bool d) {
    int pid = 0;
    if (d) {
//        signal(SIGCHLD,SIG_IGN);
        pid = fork();
        if (pid == 0) {
            const int n = setsid();
            if (-1 == n) {
                return n;
            }
            fprintf(stderr, "Run in background. pid: %d\n", getpid());
            umask(0);
            const int nret = chdir("/");
            if (nret == -1) {
                return nret;
            }
            for (auto f: {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO}) {
                close(f);
            }
            int fd = open("/dev/null", O_RDWR);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            pid = getpid();
        } else if (pid > 0) {   // parent;
            exit(0);
        } else {
        }
    } else {
        pid = getpid();
    }

    return pid;
}
