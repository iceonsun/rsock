//
// Created by Administrator on 2018/4/4.
//

#ifndef RSOCK_OS_H
#define RSOCK_OS_H

// only little endian supported
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#ifdef _WIN32
#include "os_win.h"
#else
#include "os_unix.h"
#endif

#endif //RSOCK_OS_H
