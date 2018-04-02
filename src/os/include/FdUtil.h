//
// Created on 12/4/17.
//

#ifndef RPIPE_UTIL_H
#define RPIPE_UTIL_H

#include <string>

class FdUtil {
public:
    static void CheckDgramFd(int fd);

    static void CheckStreamFd(int fd);

    static int SetNonblocking(int &fd);

    static void SetBlocking(int &fd);

    static bool FileExists(const char *fName);

    static int CreateFile(const std::string &fName, int mode = 0644);

    static int IsDir(const std::string &dirName);

    static int CreateDir(const std::string &dirName, int mode = 0755);

private:
    static void checkFdType(int fd, int type);
};


#endif //RPIPE_UTIL_H
