//
// Created by System Administrator on 12/23/17.
//


#include <string>
#include <syslog.h>
#include "../cap/cap_util.h"
#include "../client/CRawConn.h"
#include "../debug.h"
#include "../rhash.h"
#include "../cap/RCap.h"

int OnRecvCb(ssize_t nread, const rbuf_t &rbuf) {
    debug(LOG_ERR, "nread: %d", nread);
    return 0;
}

int main(int argc, char **argv) {
    std::string dev = "lo0";
    std::string targetIp = "127.0.0.1";
    std::string selfIp = "127.0.0.1";
    PortLists targetPorts = {10031, 10032,10033, 10034};   // target ports
    PortLists selfPorts = {10051, 10052, 10053, 10054};  // self ports
    int listenPort = 10030;
//    PortLists dstPorts = {20011, 20012, 20013, 20014};
    const std::string hashKey = "12345";
    const int INJECTION = LIBNET_RAW4;
    char err[LIBNET_ERRBUF_SIZE] = {0};
    libnet_t *l = libnet_init(INJECTION, dev.c_str(), err);
    if (nullptr == l) {
        debug(LOG_ERR, "libnet_init failed %s", err);
        exit(1);
    }
    debug(LOG_ERR, "version: %s", libnet_version());
    IdBufType id;
    generateIdBuf(id, hashKey);

    uv_loop_t *LOOP = uv_default_loop();
    auto cap = new RCap(dev, selfIp, targetPorts, selfPorts, targetIp);
    cap->Init();

    const int datalink = cap->Datalink();
    uint32_t targetInt = libnet_name2addr4(l, const_cast<char *>(targetIp.c_str()), LIBNET_DONT_RESOLVE);
    uint32_t selfInt = libnet_name2addr4(l, const_cast<char *>(selfIp.c_str()), LIBNET_DONT_RESOLVE);
    auto *btm = new CRawConn(l, selfInt, LOOP, hashKey, "", targetInt, datalink);
    cap->Start(IRawConn::CapInputCb, reinterpret_cast<u_char *>(btm));

    IConn *conn = new ClientConn(id, nullptr, selfIp.c_str(), listenPort, selfPorts, targetPorts, LOOP, btm, targetInt);
//    IConn *conn = new ClientConn(id, nullptr, "127.0.0.1", listenPort, srcPorts, dstPorts, LOOP, btm, dstInt);
    conn->Init();
    uv_run(LOOP, UV_RUN_DEFAULT);
    return 0;
}