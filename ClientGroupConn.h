//
// Created on 12/17/17.
//

#ifndef RSOCK_ICLIENTGROUP_H
#define RSOCK_ICLIENTGROUP_H

#include <sys/un.h>

#include "rscomm.h"

#include "IGroupConn.h"
#include "PortMapper.h"


class ClientGroupConn : public IGroupConn {
public:
    ClientGroupConn(const char *groupId, const char *listenUnPath, const char *listenUdpIp, IUINT16 listenUdpPort,
                        std::vector<IUINT16> &sourcePorts, std::vector<IUINT16> &destPorts, uv_loop_t *loop, IConn *btm);

    int Init() override;

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    int Input(ssize_t nread, const rbuf_t &rbuf) override;

    void Close() override;

    IConn *ConnOfConv(IUINT32 conv) override;

protected:
//    SPConn newConnOfOrigin(const struct sockaddr *addr);
    int send2Origin(ssize_t nread, const rbuf_t &rbuf, IUINT32 conv);
//    int udpSend(ssize_t nread, const rbuf_t &rbuf, IUINT32 conv);
int unSend(ssize_t nread, const rbuf_t &rbuf, struct sockaddr_un *addr);

    static void send_cb(uv_udp_send_t *req, int status);

    static void udpRecvCb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr,
                          unsigned flags);

private:
    static void pollCb(uv_poll_t* handle, int status, int events);

//    std::map<IUINT64 , IUINT32 > mAddrLong2Conv;
    std::map<std::string, IUINT32> mAddr2Conv;
    std::map<IUINT32, const struct sockaddr *> mConv2Origin;
//    std::map<IUINT32, SPConn> mConvMap;
    IUINT32 mConvCounter = 0;
//    SPConn mSender;

    struct omhead_t mHead;
    uv_udp_t *mUdp = nullptr;
    uv_poll_t *mUnUdp = nullptr;
//    IConn* mBtm;

//    std::vector<IUINT16> mSourcePorts;
//    std::vector<IUINT16> mDestPorts;
PortMapper *mPortMapper;
    struct sockaddr_in *mUdpAddr = nullptr;
    struct sockaddr_un *mUnAddr = nullptr;
    uv_loop_t *mLoop = nullptr;
    int mUnSock;
};


#endif //RSOCK_ICLIENTGROUP_H
