//
// Created by System Administrator on 1/16/18.
//

#include <cassert>
#include <plog/Log.h>
#include "FakeTcp.h"

FakeTcp::FakeTcp(uv_stream_t *tcp, const std::string &key) : INetConn(key){
    mTcp = tcp;
    // todo: tcpinfo must be initializated
}
int FakeTcp::Init() {
    INetConn::Init();
    assert(mTcp);

    mTcp->data = this;
    return uv_read_start(mTcp, alloc_buf, read_cb);
}

void FakeTcp::Close() {
    INetConn::Close();
    if (mTcp) {
        uv_close(reinterpret_cast<uv_handle_t *>(mTcp), close_cb);
        mTcp = nullptr;
    }
    mErrCb = nullptr;
    mAlive = false;
}

int FakeTcp::Output(ssize_t nread, const rbuf_t &rbuf) {
    int n = INetConn::Output(nread, rbuf);
    if (n >= 0) {
        mInfo.UpdateSeq(n + mInfo.seq + EncHead::GetEncBufSize());
    } else {
        mInfo.UpdateSeq(mInfo.seq + EncHead::GetEncBufSize());
    }
    return n;
}

int FakeTcp::OnRecv(ssize_t nread, const rbuf_t &buf) {
    TcpInfo *info = static_cast<TcpInfo *>(buf.data);
    assert(info);
    EncHead *hd = info->head;
    assert(hd);

    rbuf_t rbuf = {
            .base = buf.base,
            .len = (int)nread,
            .data = hd,
    };
    int n = INetConn::OnRecv(nread, rbuf);

    if (n >= 0) {
        mInfo.UpdateAck(n + info->ack + EncHead::GetEncBufSize());
    } else {
        mInfo.UpdateAck(info->ack + EncHead::GetEncBufSize());
    }

    return n;
}

void FakeTcp::read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        FakeTcp *conn = static_cast<FakeTcp *>(stream->data);
        if (nread != UV_ECANCELED) {
            conn->OnTcpError(conn, nread);
        }
    }
    free(buf->base);
}

void FakeTcp::OnTcpError(FakeTcp *conn, int err) {
    mAlive = false;
    if (mErrCb) {
        mErrCb(conn, err);
    }
}

void FakeTcp::SetErrCb(const FakeTcp::IRawTcpErrCb &cb) {
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

