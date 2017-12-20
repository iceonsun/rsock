//
// Created on 12/17/17.
//

#ifndef RSOCK_ISERVERGROUP_H
#define RSOCK_ISERVERGROUP_H


#include <vector>
#include "IGroupConn.h"
#include "SConn.h"

class ServerGroupConn : public IGroupConn {
public:
    explicit ServerGroupConn(const char *groupId, uv_loop_t *loop, IConn *btm);

    int Input(ssize_t nread, const rbuf_t &rbuf) override;


private:
    IGroupConn *newGroup(const IdBufType conn_id, const struct sockaddr *origin, IUINT8 conn_type);

private:
    std::map<std::string, IGroupConn *> mTargetMap;
    std::map<std::string, IGroupConn *> mOriginMap;
    struct sockaddr* mSelfAddr;
    struct sockaddr* mTargetAddr;
    uv_loop_t *mLoop = nullptr;
};


#endif //RSOCK_ISERVERGROUP_H
