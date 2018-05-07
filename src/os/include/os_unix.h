#ifndef OS_UNIX_H
#define OS_UNIX_H

#ifndef _WIN32

#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#define SOCKOPT_VAL_TYPE void*

#ifndef RSOCK_SOCK_BUF_TIMES
#define RSOCK_SOCK_BUF_TIMES 64 // 64 * original buf size
#endif

#endif // !_WIN32

#endif // !OS_UNIX_H
