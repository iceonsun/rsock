#include "uv.h"
#include <plog/Log.h>
#include "IPacketSyncConn.h"
#include "os_util.h"
#include "../../util/rsutil.h"
#include "rscomm.h"

IPacketSyncConn::IPacketSyncConn(uv_loop_t *loop, const Callback cb, void *obj) : ISyncConn(loop, cb, obj) {
    assert(mCb);
}

int IPacketSyncConn::Init() {
    int nret = CreateSockPair(mLoop, &mSocks);

    if (nret) {
        LOGE << "createSockPair failed: " << nret;
        return nret;
    }
    assert(mSocks.ptr);

    return 0;
}

void IPacketSyncConn::Close() {
    CloseSockPair(&mSocks);
    mLoop = nullptr;
}

int IPacketSyncConn::Send(int nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        int nret = sendto(mSocks.writeFd, rbuf.base, nread, 0, nullptr, 0);
        LOGI_IF(nret < 0) << "sendto error: " << strerror(errno);
        return nret;
    }
    return nread;
}

void IPacketSyncConn::CloseSockPair(sock_pair_t *sockPair) {
    if (sockPair) {
        if (sockPair->writeFd >= 0) {
            CloseSocket(sockPair->writeFd);
        }
        if (sockPair->ptr) {
            uv_close((uv_handle_t *) sockPair->ptr, close_cb);
        }
        if (sockPair->readFd >= 0) {
            CloseSocket(sockPair->readFd);
        }
        sockPair->readFd = -1;
        sockPair->writeFd = -1;
        sockPair->ptr = nullptr;
    }
}
