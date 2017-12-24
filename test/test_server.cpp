
#include <string>
#include <getopt.h>
#include <syslog.h>
#include "../cap/cap_util.h"
#include "../debug.h"
#include "../SRawConn.h"
#include "../rhash.h"
#include "../cap/RCap.h"

int OnRecvCb(ssize_t nread, const rbuf_t &rbuf) {
    debug(LOG_ERR, "nread: %d", nread);
    return 0;
}

int main(int argc, char **argv) {
    std::string dev = "lo0";
//    std::string srcIp = "127.0.0.1";
    std::string selfIp = "127.0.0.1";
    std::string targetIp = "127.0.0.1";
    PortLists selfPorts = {10031, 10032, 10033, 10034};  // self ports
    PortLists srcPorts = {};
    int targetPort = 10060;
//    PortLists dstPorts = {20011, 20012, 20013, 20014};
    const std::string hashKey = "12345";
    const int INJECTION = LIBNET_RAW4;
    char err[LIBNET_ERRBUF_SIZE] = {0};
    libnet_t *l = libnet_init(INJECTION, dev.c_str(), err);
    if (nullptr == l) {
        debug(LOG_ERR, "libnet_init failed %s", err);
        exit(1);
    }

    uv_loop_t *LOOP = uv_default_loop();

    auto scap = new RCap(dev, selfIp, srcPorts, selfPorts);
    scap->Init();

    int datalink = scap->Datalink();
    uint32_t dstInt = libnet_name2addr4(l, const_cast<char *>(selfIp.c_str()), LIBNET_DONT_RESOLVE);
    auto *btm = new SRawConn(l, dstInt, LOOP ,hashKey, "" , datalink);
    IdBufType id;
    generateIdBuf(id, hashKey);
    struct sockaddr_in target = {0};
    target.sin_family = AF_INET;
    target.sin_port = htons(targetPort);
    inet_aton(targetIp.c_str(), &target.sin_addr);

    IConn *conn = new ServerGroupConn(id, LOOP, btm, reinterpret_cast<const sockaddr *>(&target), selfPorts);
    conn->Init();
    scap->Start(IRawConn::CapInputCb, reinterpret_cast<u_char *>(btm));

    uv_run(LOOP, UV_RUN_DEFAULT);
    return 0;
}