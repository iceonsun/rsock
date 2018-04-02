
#include <cassert>
#include <cstring>
#include <cstdio>

#include "os.h"
#include "FdUtil.h"

void FdUtil::CheckDgramFd(int fd) {
	checkFdType(fd, SOCK_DGRAM);
}

void FdUtil::CheckStreamFd(int fd) {
	checkFdType(fd, SOCK_STREAM);
}

void FdUtil::checkFdType(int fd, int type) {
	assert(fd >= 0);
	int currType;
	socklen_t len = sizeof(socklen_t);
	getsockopt(fd, SOL_SOCKET, SO_TYPE, (SOCKOPT_VAL_TYPE) &currType, &len);
	assert(currType == type);
}