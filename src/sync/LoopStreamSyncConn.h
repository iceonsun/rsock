//
// Created by System Administrator on 5/6/18.
//

#ifndef RSOCK_STREAMDECORATOR_H
#define RSOCK_STREAMDECORATOR_H


#include "TcpStreamSyncConn.h"

class LoopStreamSyncConn : public TcpStreamSyncConn {
public:
    LoopStreamSyncConn(uv_loop_t *loop, const Callback cb, void *obj);

    int Init() override;

    void Close() override;

protected:
    int doSend(const char *buf, int nread) override;

    static void writeCb(uv_write_t* req, int status);

    static void threadCb(void *arg);
private:
    uv_loop_t *mWriteLoop = nullptr;
    uv_tcp_t *mTcp = nullptr;
    uv_thread_t mThread = 0;
};


#endif //RSOCK_STREAMDECORATOR_H
