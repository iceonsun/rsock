//
// Created on 12/16/17.
//

#ifndef RSOCK_RTASK_H
#define RSOCK_RTASK_H


#include <sys/socket.h>
#include <vector>
#include "../IConn.h"
#include "../OHead.h"

// todo: add unix sock support
class SConn : public IConn {
public:
    explicit SConn(uv_loop_t *loop, const struct sockaddr *origin, const struct sockaddr *target, IUINT32 conv);
    virtual ~SConn();

    void Close() override;

    // to origin
//    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    // to target
    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

private:
    static void udpRecvCb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr,
                          unsigned flags);
    static void sendCb(uv_udp_send_t* req, int status);
private:
    struct sockaddr_in *mTarget = nullptr;     // udp, unix sock
    struct sockaddr_in *mSelfAddr = nullptr;  // addr in server
//    struct sockaddr_in *mOrigin;
    uv_udp_t *mUdp = nullptr;
    OHead mHead;
};


#endif //RSOCK_RTASK_H
