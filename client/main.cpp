
#include <string>
#include <getopt.h>
#include <syslog.h>
#include "../cap/cap_util.h"
#include "../cap/RCap.h"
#include "CRawConn.h"
#include "../debug.h"

int OnRecvCb(ssize_t nread, const rbuf_t &rbuf) {
    debug(LOG_ERR, "nread: %d", nread);
}

int main(int argc, char **argv) {
    std::string dev = "lo0";
    std::string srcIp = "127.0.0.1";
    std::string dstIp = "127.0.0.1";
    PortLists srcPorts = {10011, 10012, 10013, 10014};
    PortLists dstPorts = {};
//    PortLists dstPorts = {20011, 20012, 20013, 20014};
    const std::string key = "12345";
    const int INJECTION = LIBNET_RAW4;
    char err[LIBNET_ERRBUF_SIZE] = {0};
    libnet_t *l = libnet_init(INJECTION, dev.c_str(), err);
    if (nullptr == l) {
        debug(LOG_ERR, "libnet_init failed %s", err);
        exit(1);
    }

    auto cap = new ICap(dev, srcIp, srcPorts, dstPorts);
    cap->Init();

    auto *rawconn = new CRawConn(l, hostIntOfIp(srcIp), uv_default_loop(), key, "", hostIntOfIp(dstIp), cap->Datalink());
    rawconn->Init();
    rawconn->SetOnRecvCb(OnRecvCb);
    cap->Start(IRawConn::CapInputCb, reinterpret_cast<u_char *>(rawconn));
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    return 0;
}