//
// Created by System Administrator on 1/16/18.
//

#ifndef RSOCK_UDPCONN_H
#define RSOCK_UDPCONN_H


#include "ConnInfo.h"
#include "INetConn.h"


// SetOutputCb is not callable
class BtmUdpConn : public INetConn {
public:
    BtmUdpConn(const std::string &key, uv_udp_t *udp, const ConnInfo &info);
//    BtmUdpConn(const std::string &key, uv_udp_t *udp);

    int Init() override;

    void Close() override;

    bool IsUdp() override;

    static void send_cb(uv_udp_send_t *req, int status);

    static void recv_cb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                        unsigned flags);

    inline ConnInfo *GetInfo() override { return &mInfo; }

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

private:
    void udpRecv(ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr);

    inline void fillInfo(const struct sockaddr *addr);

private:
    uv_udp_t *mUdp = nullptr;
    ConnInfo mInfo;
    ConnInfo mSrcInfo;
};

#endif //RSOCK_UDPCONN_H
