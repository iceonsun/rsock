#include <cassert>
#include "uv.h"
#include <plog/Log.h>
#include <os.h>
#include "UnixDgramSyncConn.h"
#include "os_util.h"
#include "../../../../util/rsutil.h"
#include "rscomm.h"

UnixDgramSyncConn::UnixDgramSyncConn(struct uv_loop_s *loop, const Callback &cb, void *obj) : IPacketSyncConn(loop, cb,
                                                                                                              obj) {
}

int UnixDgramSyncConn::CreateSockPair(uv_loop_s *loop, sock_pair_t *socks) {
    int fd2[2] = {-1, -1};
    int nret = socketpair(AF_UNIX, SOCK_DGRAM, 0, fd2);

    socks->writeFd = fd2[1];
    socks->readFd = fd2[0];
    trySetGoodBufSize(RSOCK_SOCK_BUF_TIMES, socks->readFd, socks->writeFd);

    socks->ptr = poll_dgram_fd(fd2[0], loop, pollCb, this, &nret);
    if (!socks->ptr) {
        LOGE << "poll_dgram_fd failed: " << uv_strerror(nret);
        return nret;
    }
    return 0;
}

void UnixDgramSyncConn::CloseSockPair(sock_pair_t *socks) {
    uv_poll_stop((uv_poll_t *) socks->ptr);
    IPacketSyncConn::CloseSockPair(socks);
}

void UnixDgramSyncConn::pollCb(uv_poll_t *handle, int status, int events) {
    if (status) {
        LOGE << "unix socket poll err: " << uv_strerror(status);
        return;
    }
    if (events & UV_READABLE) {
        UnixDgramSyncConn *conn = (UnixDgramSyncConn *) (handle->data);
        char buf[OM_MAX_PKT_SIZE] = {0};
        ssize_t nread = read(conn->mSocks.readFd, buf, OM_MAX_PKT_SIZE);
        if (nread > 0) {
            conn->Input(nread, new_buf(nread, buf, nullptr));
        }
    }
}

