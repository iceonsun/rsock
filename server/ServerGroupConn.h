//
// Created on 12/17/17.
//

#ifndef RSOCK_ISERVERGROUP_H
#define RSOCK_ISERVERGROUP_H


#include <vector>
#include "../IGroupConn.h"
#include "SConn.h"
#include "../cap/cap_util.h"

class ServerGroupConn : public IGroupConn {
public:
    explicit ServerGroupConn(const IdBufType &groupId, uv_loop_t *loop, SockMon* mon,  IConn *btm,
                                 const struct sockaddr *targetAddr);

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;


private:
    IGroupConn *newGroup(const IdBufType &conn_id, const struct sockaddr *peerAddr, IUINT8 conn_type, uint32_t selfInt,
                             uint32_t peerInt);

private:
    struct sockaddr* mSelfAddr;
    uv_loop_t *mLoop = nullptr;
    SockMon *mMon = nullptr;
};


#endif //RSOCK_ISERVERGROUP_H
