//
// Created on 12/4/17.
//

#ifndef RPIPE_UTIL_H
#define RPIPE_UTIL_H

class FdUtil {
public:
    static void CheckDgramFd(int fd);

    static void CheckStreamFd(int fd);

    static int SetNonblocking(int &fd);

    static void SetBlocking(int &fd);

    static bool FileExists(const char *fName);

private:
    static void checkFdType(int fd, int type);
};


#endif //RPIPE_UTIL_H
