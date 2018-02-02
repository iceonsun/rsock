//
// Created by System Administrator on 1/16/18.
//

#include <cassert>
#include <plog/Log.h>
#include "FakeTcp.h"
#include "RConn.h"
#include "../util/rsutil.h"

// todo: send 1 bytes for a specific interval to check if alive
FakeTcp::FakeTcp(uv_stream_t *tcp, const std::string &key) : INetConn(key) {
    mTcp = tcp;

    GetTcpInfo(mInfo, reinterpret_cast<uv_tcp_t *>(mTcp));

    assert(ConnInfo::BuildKey(mInfo) == key);
}

int FakeTcp::Init() {
    INetConn::Init();
    assert(mTcp);

    mTcp->data = this;
    mTimer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));

    uv_timer_init(mTcp->loop, mTimer);
    mTimer->data = this;
    uv_timer_start(mTimer, timer_cb, KEEP_ALIVE_INTERVAL, KEEP_ALIVE_INTERVAL);
    return uv_read_start(mTcp, alloc_buf, read_cb);
}

void FakeTcp::Close() {
    INetConn::Close();
    if (mTcp) {
        uv_close(reinterpret_cast<uv_handle_t *>(mTcp), close_cb);
        mTcp = nullptr;
    }
    destroyTimer();
    mErrCb = nullptr;
    mAlive = false;
}

int FakeTcp::Output(ssize_t nread, const rbuf_t &rbuf) {
    int n = INetConn::Output(nread, rbuf);
    if (n >= 0) {
        mInfo.UpdateSeq((int) nread + mInfo.seq + RConn::HEAD_SIZE);
    }
    return n;
}

int FakeTcp::OnRecv(ssize_t nread, const rbuf_t &buf) {
    TcpInfo *info = static_cast<TcpInfo *>(buf.data);
    assert(info);
    EncHead *hd = info->head;
    assert(hd);

    int n = INetConn::OnRecv(nread, buf);

    if (n >= 0) {   // todo: encapsnlate
        mInfo.UpdateAck((int) nread + mInfo.ack + RConn::HEAD_SIZE);
    }

    return n;
}

void FakeTcp::read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0 && nread != UV_ECANCELED) {
        LOGE << "err: " << uv_strerror(nread);
        FakeTcp *conn = static_cast<FakeTcp *>(stream->data);
        conn->OnTcpError(conn, nread);
    }
    free(buf->base);
}

void FakeTcp::OnTcpError(FakeTcp *conn, int err) {
    LOGE << "conn " << conn->Key() << ", err: " << err;
    mAlive = false;
    destroyTimer();
    if (mErrCb) {
        mErrCb(conn, err);
    }
}

void FakeTcp::SetErrCb(const FakeTcp::IFakeTcpErrCb &cb) {
    mErrCb = cb;
}

void FakeTcp::SetISN(uint32_t isn) {
    mInfo.UpdateSeq(isn);
}

void FakeTcp::SetAckISN(uint32_t isn) {
    mInfo.UpdateAck(isn);
}

bool FakeTcp::Alive() {
    return mAlive;
}

ConnInfo *FakeTcp::GetInfo() {
    return &mInfo;
}

bool FakeTcp::IsUdp() {
    return false;
}

void FakeTcp::timer_cb(uv_timer_t *timer) {
    FakeTcp *tcp = static_cast<FakeTcp *>(timer->data);
    tcp->checkAlive();
}

void FakeTcp::checkAlive() {
    rwrite_req_t *req = static_cast<rwrite_req_t *>(malloc(sizeof(rwrite_req_t)));
    char *base = static_cast<char *>(malloc(1));
    base[0] = 'a';
    req->buf = uv_buf_init(base, 1);
    req->write.data = this;

    uv_write(reinterpret_cast<uv_write_t *>(req), mTcp, &req->buf, 1, write_cb);
}

void FakeTcp::write_cb(uv_write_t *req, int status) {
    rwrite_req_t *req1 = reinterpret_cast<rwrite_req_t *>(req);
    if (status < 0 && status != UV_ECANCELED) {
        LOGE << "err: " << uv_strerror(status);
        FakeTcp *tcp = static_cast<FakeTcp *>(req1->write.data);
        tcp->OnTcpError(tcp, status);
    }
    free_rwrite_req(req1);
}

void FakeTcp::destroyTimer() {
    if (mTimer) {
        uv_timer_stop(mTimer);
        uv_close(reinterpret_cast<uv_handle_t *>(mTimer), close_cb);
        mTimer = nullptr;
    }
}

