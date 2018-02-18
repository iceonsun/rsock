//
// Created by System Administrator on 1/19/18.
//

#include <cassert>
#include <plog/Log.h>
#include "IGroup.h"
#include "IAppGroup.h"
#include "INetGroup.h"
#include "TcpInfo.h"
#include "RstHelper.h"
#include "../util/rhash.h"
#include "../util/rsutil.h"

using namespace std::placeholders;

IAppGroup::IAppGroup(const std::string &groupId, INetGroup *fakeNetGroup, IConn *btm,
                     const std::string &printableStr) : IGroup(groupId, btm) {
    mFakeNetGroup = fakeNetGroup;
    assert(mFakeNetGroup);

    mHead.SetIdBuf(Str2IdBuf(groupId));
    mRstHelper = new RstHelper();
    if (printableStr.empty()) {
        mPrintableStr = groupId;
    } else {
        mPrintableStr = printableStr + ", groupId: " + groupId;
    }
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

    auto fn = std::bind(&IAppGroup::onSelfNoNetConn, this, _1);
    mFakeNetGroup->SetNonConnCb(fn);

    auto out = std::bind(&IConn::Output, this, _1, _2);
    mFakeNetGroup->SetOutputCb(out);
    auto rcv = std::bind(&IConn::OnRecv, this, _1, _2);
    mFakeNetGroup->SetOnRecvCb(rcv);

    auto sendFn = std::bind(&IAppGroup::onRstConnSend, this, _1, _2, _3);
    mRstHelper->SetOutputCb(sendFn);

    auto netCb = std::bind(&IAppGroup::onPeerNetConnRst, this, _1, _2);
    mRstHelper->SetNetRecvCb(netCb);

    auto convCb = std::bind(&IAppGroup::onPeerConvRst, this, _1, _2);
    mRstHelper->SetConvRecvCb(convCb);
    return 0;
}

void IAppGroup::Close() {
    IGroup::Close();
    if (mFakeNetGroup) {
        mFakeNetGroup->Close();
        delete mFakeNetGroup;
        mFakeNetGroup = nullptr;
    }
    if (mRstHelper) {
        delete mRstHelper;
        mRstHelper = nullptr;
    }
}

int IAppGroup::Send(ssize_t nread, const rbuf_t &rbuf) {
    int n = mFakeNetGroup->Send(nread, rbuf);
    afterSend(n);
    return n;
}

int IAppGroup::Input(ssize_t nread, const rbuf_t &rbuf) {
    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
    EncHead *head = info->head;

    if (EncHead::TYPE_DATA == head->Cmd()) {
        int n = mFakeNetGroup->Input(nread, rbuf);
        if (n > 0) {
            afterInput(n);
        }
        return n;
    } else if (EncHead::IsRstFlag(head->Cmd())) {
        return mRstHelper->Input(nread, rbuf, head->Cmd());
    } else {
        LOGD << "unrecognize cmd: " << head->Cmd();
    }
    return -1;
}

void IAppGroup::Flush(uint64_t now) {
    IGroup::Flush(now);
    mFakeNetGroup->Flush(now);
}

bool IAppGroup::OnTcpFinOrRst(const TcpInfo &info) {
    return OnSelfNetConnRst(info);
}

//bool IAppGroup::OnUdpRst(const ConnInfo &info) {
//    return OnSelfNetConnRst(info);
//}

bool IAppGroup::Alive() {
    return mFakeNetGroup->Alive() && IGroup::Alive();  // if no data flows it'll report dead
}

bool IAppGroup::OnSelfNetConnRst(const ConnInfo &info) {
    auto key = ConnInfo::BuildKey(info);
    auto conn = mFakeNetGroup->ConnOfKey(key);
    if (conn) {
        LOGV << "Close conn " << key;
        mFakeNetGroup->CloseConn(conn);
        return true;
    }
    return false;
}

int IAppGroup::SendConvRst(uint32_t conv) {
    return mRstHelper->SendConvRst(conv);
}


int IAppGroup::SendNetConnRst(const ConnInfo &info) {
    return mRstHelper->SendNetConnRst(info);
}

// self don't have conn with info. send rst
int IAppGroup::onSelfNoNetConn(const ConnInfo &info) {
    return SendNetConnRst(info);
}

int IAppGroup::onRstConnSend(ssize_t nread, const rbuf_t &rbuf, uint8_t cmd) {
    mHead.SetCmd(cmd);
    const rbuf_t buf = new_buf(nread, rbuf, &mHead);
    return Send(buf.len, buf);
}

int IAppGroup::onPeerNetConnRst(const ConnInfo &src, const ConnInfo &rstInfo) {
    auto key = ConnInfo::BuildKey(rstInfo);
    auto conn = mFakeNetGroup->ConnOfKey(key);
    if (conn) {
        mFakeNetGroup->CloseConn(conn);
        return 0;
    }
    return -1;
}

int IAppGroup::onPeerConvRst(const ConnInfo &src, uint32_t rstConv) {
    auto key = ConnInfo::BuildConvKey(src.dst, rstConv);
    auto conn = ConnOfKey(key);
    if (conn) {
        CloseConn(conn);
        return 0;
    }
    return -1;
}

const std::string IAppGroup::ToStr() {
    return mPrintableStr;
}
