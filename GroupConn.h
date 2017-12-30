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
    explicit GroupConn(const IdBufType &groupId, uv_loop_t *loop, const struct sockaddr *target, const struct sockaddr *origin,
                           IUINT8 conn_type, IConn *btm);

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    void Close() override;

private:
    IConn *newConn(IUINT32 conv, const struct sockaddr *origin);

private:
    uv_loop_t *mLoop;
    struct sockaddr* mTarget;

//    struct sockaddr* mSelf = nullptr;
//    struct sockaddr *mOrigin = nullptr;
    OHead mHead;
    PortMapper mPorter;
};


#endif //RSOCK_GROUPCONN_H
