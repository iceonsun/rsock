//
// Created by System Administrator on 1/19/18.
//

#include <cassert>
#include <plog/Log.h>
#include "IGroup.h"
#include "IAppGroup.h"
#include "INetGroup.h"
#include "TcpInfo.h"

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
    nret = mFakeNetGroup->Init();
    if (nret) {
        return nret;
    }
    auto out = std::bind(&IConn::Output, this, _1, _2);
    mFakeNetGroup->SetOutputCb(out);
    auto rcv = std::bind(&IConn::OnRecv, this, _1, _2);
    mFakeNetGroup->SetOnRecvCb(rcv);

    return 0;
}

void IAppGroup::Close() {
    IGroup::Close();
    if (mFakeNetGroup) {
        mFakeNetGroup->Close();
        delete mFakeNetGroup;
        mFakeNetGroup = nullptr;
    }
}

int IAppGroup::Send(ssize_t nread, const rbuf_t &rbuf) {
    return mFakeNetGroup->Send(nread, rbuf);
}

int IAppGroup::Input(ssize_t nread, const rbuf_t &rbuf) {
    return mFakeNetGroup->Input(nread, rbuf);
}

void IAppGroup::Flush(uint64_t now) {
    IGroup::Flush(now);
    mFakeNetGroup->Flush(now);
}

bool IAppGroup::OnFinOrRst(const TcpInfo &info) {
    auto key = ConnInfo::BuildKey(info);
    auto conn = mFakeNetGroup->ConnOfKey(key);
    if (conn) {
        LOGV << "Close conn " << key;
        mFakeNetGroup->CloseConn(conn);
        return true;
    }
    return false;
}

bool IAppGroup::Alive() {
    return mFakeNetGroup->Alive();  // return FakeNetGroup::Alive will do
}
