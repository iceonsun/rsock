//
// Created by System Administrator on 1/19/18.
//

#include <cstdlib>
#include <cassert>
#include "plog/Log.h"
#include "SubGroup.h"
#include "../util/rsutil.h"
#include "../conn/ConnInfo.h"
#include "SConn.h"
#include "../EncHead.h"
#include "../util/rhash.h"

using namespace std::placeholders;

SubGroup::SubGroup(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target,
                   INetGroup *fakeNetGroup, IConn *btm) : IAppGroup(groupId, fakeNetGroup, btm) {
    mLoop = loop;
    mTarget = new_addr(target);

    mHead.id_buf = Str2IdBuf(groupId);
}

void SubGroup::Close() {
    IAppGroup::Close();
    if (mTarget) {
        free(mTarget);
        mTarget = nullptr;
    }
}

int SubGroup::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
        assert(info);
        EncHead *head = info->head;
        assert(head);

        auto key = ConnInfo::BuildConnKey(info->dst, info->dp, head->conv);
        auto conn = ConnOfKey(key);
        if (!conn) {
            conn = newConn(key, head->conv);
        }
        if (conn) {
            return conn->Input(nread, rbuf);
        }
        return -1;
    }
    return nread;
}

IConn *SubGroup::newConn(const std::string &key, IUINT32 conv) {
    IConn *conn = new SConn(key, mLoop, mTarget, conv);
    int nret = conn->Init();
    if (nret) {
        conn->Close();
        delete conn;
        return nullptr;
    }

    auto out = std::bind(&SubGroup::sconnSend, this, _1, _2);
    AddConn(conn, out, nullptr);

    LOGV << "new SConn, key: " << conn->Key();

    return conn;
}

int SubGroup::sconnSend(ssize_t nread, const rbuf_t &rbuf) {
    SConn *conn = static_cast<SConn *>(rbuf.data);
    mHead.conv = conn->Conv();
    const rbuf_t buf = {
            .base = rbuf.base,
            .len = rbuf.len,
            .data = &mHead,
    };
    return Send(nread, buf);
}
