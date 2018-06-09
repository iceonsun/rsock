//
// Created by System Administrator on 1/19/18.
//

#include <cassert>
#include <plog/Log.h>
#include "IAppGroup.h"
#include "INetGroup.h"
#include "../bean/TcpInfo.h"
#include "../callbacks/ConnReset.h"
#include "../util/rhash.h"
#include "../util/rsutil.h"
#include "../callbacks/NetConnKeepAlive.h"
#include "../src/util/KeyGenerator.h"
#include "../src/conf/ConfManager.h"
#include "../bean/RConfig.h"

using namespace std::placeholders;

IAppGroup::IAppGroup(const std::string &groupId, INetGroup *fakeNetGroup, IConn *btm) : IGroup(groupId, btm) {
    mFakeNetGroup = fakeNetGroup;
    assert(mFakeNetGroup);

    mHead.SetIdBuf(Str2IdBuf(groupId));
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

    mResetHelper = new ConnReset(this);

    mKeepAlive = new NetConnKeepAlive(this, mResetHelper,
                                      ConfManager::GetInstance()->Conf().param.keepAliveIntervalSec * 1000);
    return mKeepAlive->Init();
}

int IAppGroup::Close() {
    IGroup::Close();
    if (mFakeNetGroup) {
        mFakeNetGroup->Close();
        delete mFakeNetGroup;
        mFakeNetGroup = nullptr;
    }
    if (mKeepAlive) {
        mKeepAlive->Close();
        delete mKeepAlive;
        mKeepAlive = nullptr;
    }
    if (mResetHelper) {
        mResetHelper->Close();
        delete mResetHelper;
        mResetHelper = nullptr;
    }
    return 0;
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
        } else if (INetGroup::ERR_NO_CONN == n) {
            mResetHelper->SendNetConnRst(*info, head->ConnKey());
        }
        return n;
    } else if (EncHead::IsRstFlag(head->Cmd())) {
        return mResetHelper->Input(head->Cmd(), nread, rbuf);
    } else if (EncHead::IsKeepAliveFlag(head->Cmd())) {
        return mKeepAlive->Input(head->Cmd(), nread, rbuf);
    } else {
        LOGD << "unrecognized cmd: " << head->Cmd();
    }
    return -1;
}

void IAppGroup::Flush(uint64_t now) {
    IGroup::Flush(now);
    mFakeNetGroup->Flush(now);
}

bool IAppGroup::ProcessTcpFinOrRst(const TcpInfo &info) {
    auto key = KeyGenerator::KeyForConnInfo(info);
    auto conn = mFakeNetGroup->ConnOfIntKey(key);
    if (conn) {
        INetConn *netConn = dynamic_cast<INetConn *>(conn);
        assert(netConn);
        auto intKey = netConn->IntKey();
        return mResetHelper->OnRecvNetConnRst(info, intKey) == 0;
    }
    return false;
}

//bool IAppGroup::OnUdpRst(const ConnInfo &info) {
//    return OnSelfNetConnRst(info);
//}

bool IAppGroup::Alive() {
    if (Size()) {
        return mFakeNetGroup->Alive() && IGroup::Alive();  // if no data flows it'll report dead
    }
    return mFakeNetGroup->Alive();
}

int IAppGroup::SendConvRst(uint32_t conv) {
    return mResetHelper->SendConvRst(conv);
}

int IAppGroup::doSendCmd(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    mHead.SetCmd(cmd);
    const rbuf_t buf = new_buf(nread, rbuf, &mHead);
    int n = Send(buf.len, buf);
    mHead.SetCmd(EncHead::TYPE_DATA);   // restore back
    return n;
}

int IAppGroup::RawOutput(ssize_t nread, const rbuf_t &rbuf) {
    return Output(nread, rbuf);
}
