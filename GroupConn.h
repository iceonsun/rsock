//
// Created on 12/19/17.
//

#ifndef RSOCK_GROUPCONN_H
#define RSOCK_GROUPCONN_H


#include "IGroupConn.h"
#include "OHead.h"
#include "PortMapper.h"
#include "rstype.h"

class GroupConn : public IGroupConn {
public:
    explicit GroupConn(const IdBufType &groupId, uv_loop_t *loop, uint32_t selfInt, uint32_t targetInt,
                       const struct sockaddr *target, const sockaddr_in *peer,  SockMon* mon,IUINT8 conn_type,
                       IConn *btm);

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    void Close() override;

private:
    IConn *newConn(IUINT32 conv, const struct sockaddr *origin);

private:
    uv_loop_t *mLoop;
    struct sockaddr* mTarget;
    PortMapper mPorter;
};


#endif //RSOCK_GROUPCONN_H
