//
// Created on 12/16/17.
//

#ifndef RSOCK_RTASK_H
#define RSOCK_RTASK_H


#include <sys/socket.h>
#include <vector>
#include "IConn.h"

class SConn : public IConn {
public:
    class ISConnAddrObserver {
    public:
        virtual void OnAddrUpdated(const struct sockaddr *target, const struct sockaddr *selfAddr) = 0;
    };

    explicit SConn(IUINT32 conv, uv_loop_t *loop, const struct sockaddr *addr);
    virtual ~SConn();

    void Close() override;

    // to origin
    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    // to target
    int Input(ssize_t nread, const rbuf_t &rbuf) override;

    void RegisterObserver(ISConnAddrObserver *observer);
    void UnregisterObserver(ISConnAddrObserver *observer);

private:
    static void udpRecvCb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr,
                          unsigned flags);
    static void sendCb(uv_udp_send_t* req, int status);
    void notifyAddrChange();
private:
    std::vector<ISConnAddrObserver*> mAddrObservers;

    struct sockaddr_in *mTarget;     // udp, unix sock
    struct sockaddr_in *mSelfAddr;
    uv_udp_t *mUdp;
};


#endif //RSOCK_RTASK_H
