//
// Created on 12/17/17.
//

#include <cassert>
#include "plog/Log.h"
#include "ServerGroupConn.h"
#include "../GroupConn.h"
#include "../util/rsutil.h"

using namespace std::placeholders;

ServerGroupConn::ServerGroupConn(const IdBufType &groupId, uv_loop_t *loop, IConn *btm,
                                 const struct sockaddr *targetAddr) : IGroupConn(groupId, btm) {
    assert(loop != nullptr);
    mLoop = loop;
    mTargetAddr = new_addr(targetAddr);
}

int ServerGroupConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        OHead *head = static_cast<OHead *>(rbuf.data);
        assert(head != nullptr);
        auto addr = head->srcAddr;
        assert(addr != nullptr);

        std::string groupId = head->GroupIdStr();
        auto group = ConnOfKey(groupId);
        if (nullptr == group) {
            group = newGroup(head->GroupId(), head->srcAddr, head->ConnType());
        }

        return group->Input(nread, rbuf);
    }

    return nread;
}

IGroupConn *ServerGroupConn::newGroup(const IdBufType &conn_id, const struct sockaddr *origin, IUINT8 conn_type) {
    IGroupConn *group = new GroupConn(conn_id, mLoop, mTargetAddr, reinterpret_cast<const sockaddr_in *>(origin), conn_type, nullptr);
    AddConn(group);
    LOGV << "new group, key: " << group->Key();
    return group;
}
