//
// Created by System Administrator on 1/19/18.
//

#ifndef RSOCK_SUBCONN_H
#define RSOCK_SUBCONN_H


#include "../conn/IAppGroup.h"
#include "../EncHead.h"

class SubGroup : public IAppGroup {
public:
    SubGroup(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target,
             INetGroup *fakeNetGroup, IConn *btm);

    void Close() override;

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

private:
    IConn *newConn(const std::string &key, uint32_t conv);

    int sconnSend(ssize_t nread, const rbuf_t &rbuf);

protected:
    uv_loop_t *mLoop = nullptr;
    struct sockaddr *mTarget = nullptr;
    EncHead mHead;
};


#endif //RSOCK_SUBCONN_H
