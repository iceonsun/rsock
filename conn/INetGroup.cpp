//
// Created by System Administrator on 1/17/18.
//

#include <cstdlib>
#include <iterator>
#include <plog/Log.h>
#include "INetGroup.h"
#include "../bean/ConnInfo.h"
#include "../bean/TcpInfo.h"
#include "DefaultFakeConn.h"

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

    mDefaultFakeConn = new DefaultFakeConn();
    nret = mDefaultFakeConn->Init();
    if (nret) {
        return nret;
    }

    auto fn = std::bind(&IConn::OnRecv, this, _1, _2);
    mDefaultFakeConn->SetOnRecvCb(fn);

    return 0;
}

int INetGroup::Close() {
    IGroup::Close();
    mErrCb = nullptr;
    mHandler = nullptr; // handler will automatically remove all pending messages and tasks
    if (mDefaultFakeConn) {
        mDefaultFakeConn->Close();
        delete mDefaultFakeConn;
        mDefaultFakeConn = nullptr;
    }
    return 0;
}

int INetGroup::Input(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
        auto key = INetConn::BuildKey(*info);
        auto conn = ConnOfKey(key);
        if (!conn) {
            auto netconn = CreateNetConn(key, info);
            if (netconn) {
                AddNetConn(netconn);
            }
            conn = netconn;
        }

        if (conn) {
            int n = conn->Input(nread, rbuf);
            afterInput(n);
            return n;
        }

        LOGD << "Cannot input, no such conn: " << key << ", use default conn to process data";
        mDefaultFakeConn->Input(nread, rbuf);
        return ERR_NO_CONN;
    }
    return nread;
}

void INetGroup::AddNetConn(INetConn *conn) {
    auto out = std::bind(&IConn::Output, this, _1, _2);
    auto rcv = std::bind(&IConn::OnRecv, this, _1, _2);
    LOGD << "Add INetConn: " << conn->ToStr();
    auto err = std::bind(&INetGroup::childConnErrCb, this, _1, _2);
    conn->SetOnErrCb(err);
    AddConn(conn, out, rcv);
}

int INetGroup::Send(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        while (!mConns.empty()) {
            int n = rand() % mConns.size();
            auto it = mConns.begin();
            std::advance(it, n);
            if (it->second->Alive()) {
                int n = it->second->Send(nread, rbuf);   // udp or tcp conn
                afterSend(n);
                return n;
            }
            LOGW << "send, conn " << it->second->ToStr() << " is dead. Remove it now";
            childConnErrCb(dynamic_cast<INetConn *>(it->second), -1);
        }
        LOGE << "All conns are dead!!! Wait to reconnect";
        return ERR_NO_CONN;
    }
    return nread;
}

void INetGroup::childConnErrCb(INetConn *conn, int err) {
    if (conn) {
        LOGE << "remove conn " << conn->ToStr() << " err: " << err;
        RemoveConn(conn);     // remove it here or in handler
        auto m = mHandler->ObtainMessage(MSG_CONN_ERR, conn);
        mHandler->RemoveMessage(m);
        m.SendToTarget();
    }
}

void INetGroup::handleMessage(const Handler::Message &message) {
    switch (message.what) {
        case MSG_CONN_ERR: {
            auto *conn = static_cast<INetConn *>(message.obj);
            assert(conn);
            LOGE << "closing conn: " << conn->ToStr();
            ConnInfo *info = nullptr;
            ConnInfo *connInfo = conn->GetInfo();
            if (connInfo->IsUdp()) {
                info = new ConnInfo(*connInfo);
            } else {
                TcpInfo *tcpInfo = dynamic_cast<TcpInfo *>(connInfo);
                info = new TcpInfo(*tcpInfo);
            }
            conn->Close();  // it's already removed
            delete conn;
            netConnErr(*info);
            delete info;
            break;
        }
        default:
            break;
    }
}

// todo: server should use method do deal with wrong conn
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

// todo: replace mErrCb with a method of an abstract class
void INetGroup::netConnErr(const ConnInfo &info) {
    if (mErrCb) {
        mErrCb(info);
    }
}

IConn *INetGroup::ConnOfIntKey(IntKeyType key) {
    return ConnOfKey(INetConn::BuildKey(key));
}
