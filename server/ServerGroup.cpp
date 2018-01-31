//
// Created by System Administrator on 1/17/18.
//

#include <cassert>
#include <plog/Log.h>
#include "ServerGroup.h"
#include "../conn/ConnInfo.h"
#include "../util/rhash.h"
#include "../util/rsutil.h"
#include "SNetGroup.h"
#include "../net/INetManager.h"

using namespace std::placeholders;

ServerGroup::ServerGroup(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target, IConn *btm,
                         INetManager *netManager)
        : IGroup(groupId, btm) {
    mLoop = loop;
    mTarget = new_addr(target);
    mNetManager = netManager;
}

int ServerGroup::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
        assert(info);
        EncHead *hd = info->head;
        assert(hd);
        auto groupId = IdBuf2Str(hd->id_buf);
        auto conn = ConnOfKey(groupId);
        if (!conn) {
            conn = newConn(groupId, mLoop, mTarget);
        }
        if (conn) {
            return conn->Input(nread, rbuf);
        }
        LOGE << "no such conn";
        return -1;
    }
    return nread;

}

IConn *ServerGroup::newConn(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target) {
    auto fakenet = new SNetGroup(groupId, loop, mNetManager);
    auto conn = new SubGroup(groupId, loop, target, fakenet, nullptr);
    LOGV << "new group: " << groupId;
    if (conn->Init()) {
        conn->Close();
        delete conn;
        return nullptr;
    }
    auto out = std::bind(&IConn::Send, this, _1, _2);
    AddConn(conn, out, nullptr);
    return conn;
}
