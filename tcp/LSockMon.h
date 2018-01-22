//
// Created by System Administrator on 1/4/18.
//

#ifndef RSOCK_SOCKMON_H
#define RSOCK_SOCKMON_H


#include "SockMon.h"
#include "../util/RPortList.h"
#include "rscomm.h"

class LSockMon : public SockMon {
public:
    using NewConnCb = std::function<void(int sock, const struct sockaddr *)>;

    LSockMon(uv_loop_t *loop, const SockMonCb &cb, const std::string &ip, const RPortList &ports,
             bool noIcmp = true);

    int Init() override;

    int Close() override;

protected:
    static void newConn(uv_stream_t *server, int status);
    static void udpReadCb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr,
                          unsigned flags);
private:
    uv_tcp_t *initTcp(const std::string &ip, uint16_t port, const struct sockaddr_in *addr);
    uv_udp_t *initUdp(const std::string &ip, uint16_t port, const struct sockaddr_in *addr);

    uv_loop_t *mLoop = nullptr;
    std::vector<uv_tcp_t *> mListenedTcps;
    std::vector<uv_udp_t *> mListenedUdps;
    RPortList mPorts;
    const std::string mIp;
    bool mNoIcmp = true;
};


#endif //RSOCK_SOCKMON_H
