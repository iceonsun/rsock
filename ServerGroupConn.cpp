//
// Created on 12/17/17.
//

#include <cassert>
#include "ServerGroupConn.h"
#include "GroupConn.h"

using namespace std::placeholders;

ServerGroupConn::ServerGroupConn(const char *groupId, uv_loop_t *loop, IConn *btm) : IGroupConn(groupId, btm) {
    mLoop = loop;
}


int ServerGroupConn::Input(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        OHead *head = static_cast<OHead *>(rbuf.data);
        assert(head != nullptr);
        auto addr = head->srcAddr;
        assert(addr != nullptr);

        auto key = OHead::BuildKey(addr, head->Conv());
        std::string groupId = head->GroupId();
        auto group = ConnOfKey(groupId);
        if (nullptr == group) {
            // bug. groupid != enc.id_buf
            group = newGroup(head->GroupId(), head->srcAddr, head->ConnType());
        }

        return group->Input(nread, rbuf);
    }

    return nread;
}

IGroupConn *ServerGroupConn::newGroup(const IdBufType conn_id, const struct sockaddr *origin, IUINT8 conn_type) {
    IGroupConn *group = new GroupConn(conn_id, mLoop, mTargetAddr, std::vector<IUINT16>(), origin, conn_type, nullptr);
    AddConn(group);
    return group;
}