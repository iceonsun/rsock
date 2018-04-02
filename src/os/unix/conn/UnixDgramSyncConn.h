#ifndef UNIX_DGRAM_SYNC_CONN_H
#define UNIX_DGRAM_SYNC_CONN_H

#include "../../../sync/IPacketSyncConn.h"

class UnixDgramSyncConn : public IPacketSyncConn {
public:
    UnixDgramSyncConn(struct uv_loop_s *loop, const Callback &cb, void *obj);

    int CreateSockPair(struct uv_loop_s *loop, sock_pair_t *socks) override;

    void CloseSockPair(sock_pair_t *socks) override;

protected:
    static void pollCb(uv_poll_t *handle, int status, int events);
};

#endif // UNIX_DGRAM_SYNC_CONN_H
