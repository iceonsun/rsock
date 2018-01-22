//
// Created by System Administrator on 1/19/18.
//

#include <cassert>
#include "IGroup.h"
#include "IAppGroup.h"
#include "INetGroup.h"

using namespace std::placeholders;

IAppGroup::IAppGroup(const std::string &groupId, INetGroup *fakeNetGroup, IConn *btm) : IGroup(groupId, btm) {
    mFakeNetGroup = fakeNetGroup;
    assert(mFakeNetGroup);
}

int IAppGroup::Init() {
    int nret = IGroup::Init();
    if (nret) {
        return nret;
    }
    if (mFakeNetGroup) {
        nret = mFakeNetGroup->Init();
        if (nret) {
            return nret;
        }
        auto out = std::bind(&IConn::Output, this, _1, _2);
        mFakeNetGroup->SetOutputCb(out);
        auto rcv = std::bind(&IConn::OnRecv, this, _1, _2);
        mFakeNetGroup->SetOnRecvCb(rcv);
    }
    return 0;
}

void IAppGroup::Close() {
    IGroup::Close();
    mFakeNetGroup->Close();
}

int IAppGroup::Send(ssize_t nread, const rbuf_t &rbuf) {
    return mFakeNetGroup->Send(nread, rbuf);
}

void IAppGroup::AddNetConn(INetConn *conn) {
    mFakeNetGroup->AddNetConn(conn);
}

int IAppGroup::Input(ssize_t nread, const rbuf_t &rbuf) {
    return mFakeNetGroup->Input(nread, rbuf);
}

void IAppGroup::RemoveNetConn(INetConn *conn) {
    mFakeNetGroup->RemoveConn(conn, true);
}

INetConn *IAppGroup::NetConnOfKey(const std::string &key) {
    return dynamic_cast<INetConn *>(mFakeNetGroup->ConnOfKey(key));
}
