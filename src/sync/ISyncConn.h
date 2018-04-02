#ifndef ISYNC_CONN_H
#define ISYNC_CONN_H

#include "rcommon.h"
#include "uv.h"

class ISyncConn {
public:
    using Callback = int(*)(void *obj, ssize_t nread, const rbuf_t &rbuf);

    explicit ISyncConn(uv_loop_t *loop, const Callback cb, void *obj);

    virtual ~ISyncConn() = default;

    virtual int Init() = 0;

    virtual void Close() = 0;

    virtual int Send(int nread, const rbuf_t &rbuf) = 0;

    static void trySetGoodBufSize(int NTIMES, int readSock, int writeSock);

    static void trySetGoodBufSize(int NTIMES, uv_handle_t *readHandle, uv_handle_t *writeHandle);

protected:
    // use cb
    virtual int Input(ssize_t nread, const rbuf_t &rbuf);

protected:
    struct uv_loop_s *mLoop = nullptr;
    const Callback mCb = nullptr;
    void *mObj = nullptr;
};

#endif // !ISYNC_CONN_H
