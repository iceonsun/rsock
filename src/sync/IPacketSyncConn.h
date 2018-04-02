//
// Created by System Administrator on 5/6/18.
//

#ifndef RSOCK_IPACKETSYNCCONN_H
#define RSOCK_IPACKETSYNCCONN_H


#include "ISyncConn.h"

class IPacketSyncConn : public ISyncConn {
public:
    struct sock_pair_t {
        int writeFd = -1;
        int readFd = -1;
        void *ptr = nullptr;
    };

    IPacketSyncConn(uv_loop_t *loop, const Callback cb, void *obj);

    int Init() override;

    void Close() override;

    int Send(int nread, const rbuf_t &rbuf) override;

protected:
    virtual int CreateSockPair(struct uv_loop_s *loop, sock_pair_t *socks) = 0;

    // default: close(writeFd); uv_close((uv_handle_t*) readPtr);
    virtual void CloseSockPair(sock_pair_t *socks);

protected:
    sock_pair_t mSocks;

};


#endif //RSOCK_IPACKETSYNCCONN_H
