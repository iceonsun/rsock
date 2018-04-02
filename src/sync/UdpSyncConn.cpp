#include "os.h"
#include "uv.h"
#include <plog/Log.h>
#include "UdpSyncConn.h"
#include "../../util/rsutil.h"
#include "os_util.h"
#include "rscomm.h"

UdpSyncConn::UdpSyncConn(struct uv_loop_s *loop, const Callback cb, void *obj) : IPacketSyncConn(loop, cb, obj) {
}


void UdpSyncConn::udpCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const sockaddr *addr, unsigned flags) {
    UdpSyncConn *conn = (UdpSyncConn *) handle->data;
    if (nread > 0) {
        const rbuf_t rbuf = new_buf(nread, buf->base, nullptr);
        conn->Input(nread, rbuf);
    }
    free(buf->base);
}


int UdpSyncConn::CreateSockPair(uv_loop_s *loop, sock_pair_t *socks) {
    struct sockaddr_in addr = {0};
    uv_ip4_addr("127.0.0.1", 0, &addr);
    uv_udp_t *udp = (uv_udp_t *) (malloc(sizeof(uv_udp_t)));

    uv_udp_init(loop, udp);

    udp->data = this;

    int nret = uv_udp_bind(udp, (SA *) (&addr), 0);
    if (nret) {
        LOGE << "uv_udp_bind failed, err: " << uv_strerror(nret);
        free(udp);
        return nret;
    }

    nret = uv_udp_recv_start(udp, alloc_buf, udpCb);
    if (nret) {
        uv_close(reinterpret_cast<uv_handle_t *>(udp), close_cb);
        return nret;
    }

    int writeFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int socklen = sizeof(addr);
    nret = uv_udp_getsockname(udp, (SA *) (&addr), &socklen);
    if (nret) {
        LOGE << "uv_udp_getsockname on udp failed: " << Addr2Str((SA *) (&addr)) << ", err: " << uv_strerror(nret);
        uv_close((uv_handle_t *) (udp), close_cb);
        return nret;
    }

    nret = connect(writeFd, (SA *) (&addr), sizeof(addr));
    if (nret) {
        LOGE << "connect on udp failed: " << Addr2Str((SA *) (&addr)) << ", err: " << uv_strerror(nret);
        uv_close((uv_handle_t *) (udp), close_cb);
        return nret;
    }

    LOGD << "readFd addr: " << Addr2Str((SA *) &addr);

    trySetGoodBufSize(RSOCK_SOCK_BUF_TIMES, (uv_handle_t*)udp, nullptr);
    trySetGoodBufSize(RSOCK_SOCK_BUF_TIMES, -1, writeFd);
    socks->writeFd = writeFd;
    socks->ptr = udp;

    return 0;
}
