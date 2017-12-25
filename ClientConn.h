//
// Created on 12/17/17.
//

#ifndef RSOCK_ICLIENTGROUP_H
#define RSOCK_ICLIENTGROUP_H

#include <sys/un.h>

#include "rscomm.h"

#include "IGroupConn.h"
#include "PortMapper.h"
#include "OHead.h"
#include "rstype.h"


class ClientConn : public IGroupConn {
public:
    ClientConn(const IdBufType &groupId, const char *listenUnPath, const char *listenUdpIp,
                   IUINT16 listenUdpPort, std::vector<IUINT16> &sourcePorts, std::vector<IUINT16> &destPorts,
                   uv_loop_t *loop, IConn *btm, uint32_t bigDst);
    int Init() override;

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    void Close() override;

    // empty implementation
    void AddConn(IConn *conn, bool bindOutput) override {};

    // emply implementation
    void RemoveConn(IConn *conn) override {};

private:
    int send2Origin(ssize_t nread, const rbuf_t &rbuf, const sockaddr *origin);

    int unSendOrigin(ssize_t nread, const rbuf_t &rbuf, struct sockaddr_un *addr);

    static void send_cb(uv_udp_send_t *req, int status);

    static void udpRecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                          unsigned flags);

private:
    static void pollCb(uv_poll_t *handle, int status, int events);

    std::map<std::string, IUINT32> mAddr2Conv;
    std::map<IUINT32, struct sockaddr *> mConv2Origin;
    IUINT32 mConvCounter = 1;

    OHead mHead;
    uv_udp_t *mUdp = nullptr;
    uv_poll_t *mUnPoll = nullptr;
    int mUnSock;

    struct sockaddr_in *mUdpAddr = nullptr;
    struct sockaddr_un *mUnAddr = nullptr;

    PortMapper mPortMapper; // todo initialize this.
    uv_loop_t *mLoop = nullptr;
};


#endif //RSOCK_ICLIENTGROUP_H
