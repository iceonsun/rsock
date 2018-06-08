//
// Created by System Administrator on 5/6/18.
//

#include <plog/Log.h>
#include "LoopStreamSyncConn.h"
#include "../../util/UvUtil.h"

LoopStreamSyncConn::LoopStreamSyncConn(uv_loop_t *loop, const Callback cb, void *obj)
        : TcpStreamSyncConn(loop, cb, obj) {
}

int LoopStreamSyncConn::Init() {
    int nret = TcpStreamSyncConn::Init();
    if (nret) {
        return nret;
    }

    mWriteLoop = static_cast<uv_loop_t *>(malloc(sizeof(uv_loop_t)));
    nret = uv_loop_init(mWriteLoop);
    if (nret) {
        LOGE << "uv_loop_init failed: " << uv_strerror(nret);
        free(mWriteLoop);
        mWriteLoop = nullptr;
        return nret;
    }

    int writeSock = getWriteSock();
    if (writeSock < 0) {
        LOGE << "writeSock < 0: " << writeSock;
        return -1;
    }

    mTcp = static_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));
    nret = uv_tcp_init(mWriteLoop, mTcp);
    assert(nret == 0);
    nret = uv_tcp_open(mTcp, writeSock);
    if (nret) {
        LOGE << "uv_tcp_open failed: " << uv_strerror(nret);
        uv_close(reinterpret_cast<uv_handle_t *>(mTcp), close_cb);
        mTcp = nullptr;
        return nret;
    }

    setWriteSock(-1);
    uv_thread_create(&mThread, threadCb, mWriteLoop);

    return 0;
}

void LoopStreamSyncConn::Close() {
    if (mThread != 0) {
        uv_thread_join(&mThread);
        mThread = 0;
    }
    if (mWriteLoop) {
        UvUtil::stop_and_close_loop_fully(mWriteLoop);
        free(mWriteLoop);
        mWriteLoop = nullptr;
        // don't delete it here
    }
    TcpStreamSyncConn::Close();
}

int LoopStreamSyncConn::doSend(const char *buf, int nread) {
    if (nread > 0 && mWriteLoop) {
        rwrite_req_t *req = static_cast<rwrite_req_t *>(malloc(sizeof(rwrite_req_t)));
        char *base = (char*) malloc(nread);
        memcpy(base, buf, nread);
        req->buf = uv_buf_init(base, nread);
        uv_write(reinterpret_cast<uv_write_t *>(req), reinterpret_cast<uv_stream_t *>(mTcp), &req->buf, 1, writeCb);
    }
    return nread;
}

void LoopStreamSyncConn::writeCb(uv_write_t *req, int status) {
    if (status) {
        LOGE << "writeFailed: " << uv_strerror(status);
    }
    free_rwrite_req(reinterpret_cast<rwrite_req_t *>(req));
}

void LoopStreamSyncConn::threadCb(void *arg) {
    uv_loop_t *loop = static_cast<uv_loop_t *>(arg);
    if (loop) {
        uv_run(loop, UV_RUN_DEFAULT);
    }
}
