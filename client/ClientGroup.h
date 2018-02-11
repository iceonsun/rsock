//
// Created on 12/17/17.
//

#ifndef RSOCK_ICLIENTGROUP_H
#define RSOCK_ICLIENTGROUP_H

#include <sys/un.h>

#include "../conn/IAppGroup.h"
#include "../EncHead.h"

class CConn;

class RPortList;

class ClientGroup : public IAppGroup {
public:
    ClientGroup(const std::string &groupId, const std::string &listenUnPath, const std::string &listenUdpIp,
                uint16_t listenUdpPort, uv_loop_t *loop, INetGroup *fakeGroup, IConn *btm);

    int Init() override;

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    void Close() override;

    bool RemoveConn(IConn *conn) override;

private:
    int subconnRecv(ssize_t nread, const rbuf_t &rbuf);

    int send2Origin(ssize_t nread, const rbuf_t &rbuf, const sockaddr *origin);

    int unSendOrigin(ssize_t nread, const rbuf_t &rbuf, struct sockaddr_un *addr);

    static void send_cb(uv_udp_send_t *req, int status);

    static void udpRecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                          unsigned flags);

    void onLocalRecv(ssize_t nread, const char *base, const struct sockaddr *addr);

    CConn *newConn(const std::string &key, const struct sockaddr *addr, uint32_t conv);

    static void pollCb(uv_poll_t *handle, int status, int events);

    int cconSend(ssize_t nread, const rbuf_t &rbuf);


private:

    uint32_t mConvCounter = 1;

    uv_udp_t *mUdp = nullptr;
    uv_poll_t *mUnPoll = nullptr;
    int mUnSock;

    struct sockaddr_in *mUdpAddr = nullptr;
    struct sockaddr_un *mUnAddr = nullptr;

    uv_loop_t *mLoop = nullptr;
    std::map<uint32_t, CConn *> mConvMap;
    EncHead mHead;
};


#endif //RSOCK_ICLIENTGROUP_H
