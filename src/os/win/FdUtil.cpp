//
// Created on 12/4/17.
//

#include <cassert>
#include <cstring>
#include <cstdio>

#include "os.h"
#include "FdUtil.h"

static int setBlockingMode(int &fd, u_long mode) {
	return ioctlsocket(fd, FIONBIO, &mode);
}

int FdUtil::SetNonblocking(int &fd) {
	return setBlockingMode(fd, 1);
}

void FdUtil::SetBlocking(int &fd) {
	setBlockingMode(fd, 0);
}

bool FdUtil::FileExists(const char *fName) {
	return _access(fName, F_OK) == 0;
}

int FdUtil::CreateFile(const std::string &fName, int mode) {
	if (!FileExists(fName.c_str())) {
		HANDLE fileHandle = ::CreateFile(fName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_NEW, 0, 0);
		if (fileHandle == INVALID_HANDLE_VALUE) {
			fprintf(stderr, "failed to create file: %s, err: %d\n", fName.c_str(), GetLastError());
			return -1;
		}

		CloseHandle(fileHandle);
	}

	return 1;
}

int FdUtil::IsDir(const std::string &dirName) {
	DWORD dwAttrib = GetFileAttributes(dirName.c_str());

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int FdUtil::CreateDir(const std::string &dirName, int mode) {
	return CreateDirectory(dirName.c_str(), NULL);
}