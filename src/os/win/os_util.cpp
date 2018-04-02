
#include <time.h>

#include "os.h"

#include "os_util.h"

static struct WSAData gs_wsaData;

int os_init_onstartup() {
	return WSAStartup(MAKEWORD(2, 2), &gs_wsaData);
}

int os_clean() {
	return WSACleanup();
}

void win_gettimeofday(struct timeval *tp, void *tzp) {
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
}


void rgettimeofday(struct timeval *val) {
	win_gettimeofday(val, 0);
}

void CloseSocket(int sock) {
	if (sock >= 0) {
		::closesocket(sock);
	}
}

int GetPrevSockErr() {
	return WSAGetLastError();
}