#ifndef OS_UTIL_H
#define OS_UTIL_H

#include <string>

struct timeval;

struct in_addr;

struct uv_loop_s;

int os_init_onstartup();

int os_clean();

void rgettimeofday(struct timeval *val);

void CloseSocket(int sock);

int GetPrevSockErr();

#endif // !OS_H
