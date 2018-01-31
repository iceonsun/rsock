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

int INetGroup::Input(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
        auto key = ConnInfo::BuildKey(*info);
        auto conn = ConnOfKey(key);
        if (!conn) {
            auto netconn = CreateNewConn(key, info);
            if (netconn) {
                AddNetConn(netconn);
            }
            conn = netconn;
        }

        if (conn) {
            return conn->Input(nread, rbuf);
        }
        LOGD << "no such conn: " << key;
        return -1;
    }
    return nread;
}

void INetGroup::AddConn(IConn *conn, const IConn::IConnCb &outCb, const IConn::IConnCb &recvCb) {
    IGroup::AddConn(conn, outCb, recvCb);
}

void INetGroup::AddNetConn(INetConn *conn) {
    auto out = std::bind(&IConn::Output, this, _1, _2);
    auto rcv = std::bind(&IConn::OnRecv, this, _1, _2);
    AddConn(conn, out, rcv);
}

// 找send的conn有问题
int INetGroup::Send(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        std::map<std::string, IConn *> fails;
        while (fails.size() < mConns.size()) {
            int n = rand() % mConns.size();
            auto it = mConns.begin();
            std::advance(it, n);
            if (it->second->Alive()) {
                return it->second->Send(nread, rbuf);   // udp or tcp conn
            }
            LOGW << "conn " << it->second->Key() << " is dead but not removed";
            fails.emplace(it->first, it->second);
        }
        LOGE << "all conns are dead";
        return -1;
    }
    return nread;
}
