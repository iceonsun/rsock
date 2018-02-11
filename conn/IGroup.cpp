//
// Created on 12/16/17.
//

#include <cassert>
#include <vector>
#include "plog/Log.h"
#include "IGroup.h"

using namespace std::placeholders;

IGroup::IGroup(const std::string &groupId, IConn *btm)
        : IConn(groupId) {
    mBtm = btm;
}

int IGroup::Init() {
    int nret = IConn::Init();
    if (nret) {
        LOGE << "IConn::Init failed: " << nret;
        return nret;
    }

    if (mBtm) {
        nret = mBtm->Init();
        if (nret) {
            LOGE << "btm conn init failed: " << nret;
            return nret;
        }

        auto out = std::bind(&IConn::Send, mBtm, std::placeholders::_1, std::placeholders::_2);
        SetOutputCb(out);

        auto in = std::bind(&IConn::Input, this, std::placeholders::_1, std::placeholders::_2);
        mBtm->SetOnRecvCb(in);
    }
    return 0;
}

void IGroup::Close() {
    IConn::Close();
    if (mBtm) {
        mBtm->Close();
        delete mBtm;
        mBtm = nullptr;
    }
    if (!mConns.empty()) {
        for (auto e: mConns) {
            e.second->Close();
            delete e.second;
        }
        mConns.clear();
    }
}

bool IGroup::RemoveConn(IConn *conn) {
    auto n = mConns.erase(conn->Key());
    conn->SetOnRecvCb(nullptr);
    conn->SetOutputCb(nullptr);
    return n != 0;
}

IConn *IGroup::ConnOfKey(const std::string &key) {
    auto it = mConns.find(key);
    return (it != mConns.end()) ? it->second : nullptr;
}

void IGroup::AddConn(IConn *conn, const IConn::IConnCb &outCb, const IConn::IConnCb &recvCb) {
    mConns.insert({conn->Key(), conn});
    if (outCb) {
        conn->SetOutputCb(outCb);
    }
    if (recvCb) {
        conn->SetOnRecvCb(recvCb);
    }
}

void IGroup::Flush(uint64_t now) {
    std::vector<std::pair<std::string, IConn *>> fails;

    for (auto &e: mConns) {
        if (!e.second->Alive()) {
            fails.emplace_back(e);
        }
    }

    // remove dead conns first
    for (auto &e: fails) {
        LOGV << "conn " << e.second->Key() << " is dead";
        if (!OnConnDead(e.second)) {
            CloseConn(e.second);
        }
    }

    if (mConns.empty()) {
        LOGD << "group: " << Key() << ", all conns are dead";
    }

    // then flush
    for (auto &e: mConns) {
        e.second->Flush(now);
    }
}

bool IGroup::Alive() {
    return IConn::Alive();
//    if (!IConn::Alive()) {
//        return false;
//    }
//
//    for (auto &e: mConns) {
//        if (e.second->Alive()) {
//            return true;
//        }
//    }
//    return false;
}

std::map<std::string, IConn *> &IGroup::GetAllConns() {
    return mConns;
}

bool IGroup::CloseConn(IConn *conn) {
    if (conn) {
        LOGD << "closing conn " << conn->Key();
        assert(conn->Key() != Key());
        assert(conn == ConnOfKey(conn->Key()));
        mConns.erase(conn->Key());
        conn->Close();
        delete conn;
        return true;
    }
    return false;
}
