//
// Created on 12/4/17.
//

#include <cassert>
#include <cstring>
#include <cstdio>

#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
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
    getsockopt(fd, SOL_SOCKET, SO_TYPE, &currType, &len);
    assert(currType == type);
}

int FdUtil::SetNonblocking(int &fd) {
    int oflag = fcntl(fd, F_GETFL, 0);
    int newFlag = oflag | O_NONBLOCK;
    return fcntl(fd, F_SETFL, newFlag);
}

void FdUtil::SetBlocking(int &fd) {
    int oflag = fcntl(fd, F_GETFL, 0);
    int newFlag = oflag & (~O_NONBLOCK);
    fcntl(fd, F_SETFL, newFlag);
}

bool FdUtil::FileExists(const char *fName) {
    return access(fName, F_OK) == 0;
}

int FdUtil::CreateFile(const std::string &fName, int mode) {
    auto pos = fName.rfind('/');
    int nret = 0;
    if (pos != std::string::npos) {
        auto dirName = fName.substr(0, pos);
        if (!FileExists(dirName.c_str()) || 1 != IsDir(dirName)) {
            nret = CreateDir(dirName, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
            if (nret) {
                return nret;
            }
        }
        nret = open(fName.c_str(), O_WRONLY|O_CREAT, mode);
        if (nret >= 0) {
            close(nret);    // just close the fd descriptor
        }
    }
    return nret;
}

int FdUtil::IsDir(const std::string &dirName) {
    struct stat info = {0};
    int nret = stat(dirName.c_str(), &info);
    if (0 == nret) {
        return S_ISDIR(info.st_mode);
    }
    return nret;
}

int FdUtil::CreateDir(const std::string &dirName, int mode) {
    return mkdir(dirName.c_str(), mode);
}