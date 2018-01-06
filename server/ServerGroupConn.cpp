//
// Created on 12/17/17.
//

#include <cassert>
#include "plog/Log.h"
#include "ServerGroupConn.h"
#include "../GroupConn.h"
#include "../util/rsutil.h"

using namespace std::placeholders;

ServerGroupConn::ServerGroupConn(const IdBufType &groupId, uv_loop_t *loop, SockMon *mon, IConn *btm,
                                 const struct sockaddr *targetAddr) : IGroupConn(groupId, 0, 0, nullptr, btm) {
    assert(loop != nullptr);
    mLoop = loop;
    mSelfAddr = new_addr(targetAddr);
    mMon = mon;
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
            group = newGroup(head->GroupId(), head->srcAddr, head->ConnType(), head->Dst(), head->Src());
        }

        return group->Input(nread, rbuf);
    }

    return nread;
}

IGroupConn *ServerGroupConn::newGroup(const IdBufType &conn_id, const struct sockaddr *peerAddr, IUINT8 conn_type, 
                                      uint32_t selfInt, uint32_t peerInt) {
    IGroupConn *group = new GroupConn(conn_id, mLoop,  selfInt, peerInt, mSelfAddr, reinterpret_cast<const sockaddr_in *>(peerAddr),
                                      mMon, conn_type, nullptr);
    AddConn(group);
    LOGV << "new group, key: " << group->Key();
    return group;
}
