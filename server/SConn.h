//
// Created on 12/16/17.
//

#ifndef RSOCK_RTASK_H
#define RSOCK_RTASK_H

struct sockaddr;
struct sockaddr_in;

#include <rscomm.h>
#include "../conn/IConn.h"


// todo: add unix sock support
class SConn : public IConn {
public:
    explicit SConn(const std::string &key, uv_loop_t *loop, const SA *target, uint32_t conv);

    int Init() override;

    void Close() override;

    // to target
    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    uint32_t Conv();

private:
    static void udpRecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const SA *addr,
                          unsigned flags);

    static void sendCb(uv_udp_send_t *req, int status);

private:
    SA4 *mTarget = nullptr;     // udp, unix sock
    SA4 *mSelfAddr = nullptr;  // addr in server
//    struct sockaddr_in *mOrigin;
    uv_udp_t *mUdp = nullptr;
    uint32_t mConv = 0;
};


#endif //RSOCK_RTASK_H
