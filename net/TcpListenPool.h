//
// Created by System Administrator on 1/27/18.
//

#ifndef RSOCK_NETLISTENPOOL_H
#define RSOCK_NETLISTENPOOL_H

#include <functional>
#include <rscomm.h>

#include "uv.h"
#include "../util/RPortList.h"

class TcpListenPool {
public:
    using NewConnCb = std::function<void(uv_tcp_t *client)>;

    explicit TcpListenPool(const RPortList &ports, const std::string &ip, uv_loop_t *loop);

    virtual int Init();

    virtual void Close();

    void SetNewConnCb(const NewConnCb &cb);

protected:
    // if status != 0, stream is is listened tcp that occur error. otherwise new connection
    virtual void newConn(uv_stream_t *stream, int status);

private:
    void clearPool();

    static void new_conn_cb(uv_stream_t *server, int status);

    uv_tcp_t *initTcp(const SA4 *addr);

private:
    RPortList mPorts;
    NewConnCb mNewConnCb = nullptr;

    uv_loop_t *mLoop = nullptr;
    std::vector<uv_tcp_t *> mPool;
    const std::string mIp;
};


#endif //RSOCK_NETLISTENPOOL_H
