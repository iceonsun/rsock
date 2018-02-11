//
// Created by System Administrator on 1/17/18.
//

#include <cstdlib>
#include <iterator>
#include <plog/Log.h>
#include "INetGroup.h"
#include "ConnInfo.h"

using namespace std::placeholders;

INetGroup::INetGroup(const std::string &groupId, uv_loop_t *loop)
        : IGroup(groupId, nullptr) {
    mLoop = loop;
}

int INetGroup::Init() {
    int nret = IGroup::Init();
    if (nret) {
        return nret;
    }

    auto cb = std::bind(&INetGroup::handleMessage, this, std::placeholders::_1);
    mHandler = Handler::NewHandler(mLoop, cb);
    setupTimer();
    return 0;
}

void INetGroup::Close() {
    IGroup::Close();
    destroyTimer();
    mErrCb = nullptr;
    mHandler = nullptr; // handler will automatically remove all pending messages and tasks
}

int INetGroup::Input(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
        auto key = ConnInfo::BuildKey(*info);
        auto conn = ConnOfKey(key);
        if (!conn) {
            auto netconn = CreateNetConn(key, info);
            if (netconn) {
                LOGD << "new INetConn: " << netconn->Key();
                AddNetConn(netconn);
            }
            conn = netconn;
        }

        if (conn) {
            int n = conn->Input(nread, rbuf);
            afterInput(n);
            return n;
        }
        LOGD << "Cannot input, no such conn: " << key;
        return -1;
    }
    return nread;
}

void INetGroup::AddNetConn(INetConn *conn) {
    auto out = std::bind(&IConn::Output, this, _1, _2);
    auto rcv = std::bind(&IConn::OnRecv, this, _1, _2);
    LOGD << "Add INetConn: " << conn->Key();
    auto err = std::bind(&INetGroup::childConnErrCb, this, _1, _2);
    conn->SetOnErrCb(err);
    AddConn(conn, out, rcv);
}

// 找send的conn有问题 todo: remove failed conns
int INetGroup::Send(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        decltype(mConns) fails;
        while (fails.size() < mConns.size()) {
            int n = rand() % mConns.size();
            auto it = mConns.begin();
            std::advance(it, n);
            if (it->second->Alive()) {
                int n = it->second->Send(nread, rbuf);   // udp or tcp conn
                afterSend(n);
                return n;
            }
            LOGW << "send, conn " << it->second->Key() << " is dead but not removed";
            fails.emplace(it->first, it->second);
        }
        for (auto &e: fails) {
            childConnErrCb(dynamic_cast<INetConn *>(e.second), -1);
        }
        return -1;
    }
    return nread;
}

void INetGroup::childConnErrCb(INetConn *conn, int err) {
    if (conn) {
        LOGE << "remove conn " << conn->Key() << " err: " << err;
        RemoveConn(conn);     // remove it here or in handler
        auto m = mHandler->ObtainMessage(CONN_ERR, err, "", conn);
        mHandler->RemoveMessage(m);
        m.SendToTarget();
    }
}

void INetGroup::handleMessage(const Handler::Message &message) {
    switch (message.what) {
        case CONN_ERR: {
            auto *conn = static_cast<INetConn *>(message.obj);
            assert(conn);

            LOGE << "closing conn: " << conn->Key() << ", err: " << message.what;
            ConnInfo info(*conn->GetInfo());
            conn->Close();  // it's already removed
            delete conn;
            netConnErr(info);
            break;
        }
        default:
            break;
    }
}

void INetGroup::netConnErr(const ConnInfo &info) {
    if (mErrCb) {
        mErrCb(info);
    }
}

void INetGroup::SetNetConnErrCb(const NetConnErrCb &cb) {
    mErrCb = cb;
}

bool INetGroup::OnConnDead(IConn *conn) {
    INetConn *c = dynamic_cast<INetConn *>(conn);
    if (c) {
        childConnErrCb(c, -1);
        return true;
    }
    return false;
}

void INetGroup::setupTimer() {
    if (mFlushTimer) {
        mFlushTimer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
        uv_timer_init(mLoop, mFlushTimer);
        mFlushTimer->data = this;
        uv_timer_start(mFlushTimer, timer_cb, FLUSH_INTERVAL * 4, FLUSH_INTERVAL);
    }
}

void INetGroup::destroyTimer() {
    if (mFlushTimer) {
        uv_timer_stop(mFlushTimer);
        uv_close(reinterpret_cast<uv_handle_t *>(mFlushTimer), close_cb);
        mFlushTimer = nullptr;
    }
}

void INetGroup::timer_cb(uv_timer_t *timer) {
    INetGroup *group = static_cast<INetGroup *>(timer->data);
    group->Flush(uv_now(group->mLoop));
}
