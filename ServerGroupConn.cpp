//
// Created on 12/17/17.
//

#include <cassert>
#include <syslog.h>
#include "ServerGroupConn.h"
#include "GroupConn.h"
#include "rsutil.h"
#include "debug.h"
#include "cap/cap_util.h"

using namespace std::placeholders;

ServerGroupConn::ServerGroupConn(const IdBufType &groupId, uv_loop_t *loop, IConn *btm, const struct sockaddr *targetAddr,
                                 const PortLists &selfPorts) : IGroupConn(groupId, btm), mSelfPorts(selfPorts) {
    mLoop = loop;
    mTargetAddr = new_addr(targetAddr);
//    debug(LOG_ERR, "server target: %s:%d", t)
}


int ServerGroupConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        OHead *head = static_cast<OHead *>(rbuf.data);
        assert(head != nullptr);
        auto addr = head->srcAddr;
        assert(addr != nullptr);

        auto key = OHead::BuildKey(addr, head->Conv());
        std::string groupId = head->GroupIdStr();
        auto group = ConnOfKey(groupId);
        if (nullptr == group) {
            // bug. groupid != enc.id_buf
            group = newGroup(head->GroupId(), head->srcAddr, head->ConnType());
        }
#ifndef NNDEBUG
        else {
            debug(LOG_ERR, "input %d bytes to. old group: %s", group->Key().c_str());
        }
#endif

        return group->Input(nread, rbuf);
    }

    return nread;
}

IGroupConn *ServerGroupConn::newGroup(const IdBufType &conn_id, const struct sockaddr *origin, IUINT8 conn_type) {
    IGroupConn *group = new GroupConn(conn_id, mLoop, mTargetAddr, mSelfPorts, origin, conn_type, nullptr);
    AddConn(group);
#ifndef NNDEBUG
    debug(LOG_ERR, "new group, key: %s", group->Key().c_str());
#endif
    return group;
}