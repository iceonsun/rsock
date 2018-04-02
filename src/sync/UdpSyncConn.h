#ifndef UDP_SYNC_CONN_H
#define UDP_SYNC_CONN_H


#include "IPacketSyncConn.h"

class UdpSyncConn : public IPacketSyncConn {
public:
	UdpSyncConn(struct uv_loop_s *loop, const Callback cb, void *obj);

	int CreateSockPair(struct uv_loop_s *loop, sock_pair_t *socks) override;

protected:
	static void udpCb(struct uv_udp_s* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);

};

#endif // !UDP_SYNC_CONN_H
