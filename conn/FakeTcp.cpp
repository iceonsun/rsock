//
// Created by System Administrator on 1/16/18.
//

#include <cassert>
#include <plog/Log.h>
#include "FakeTcp.h"
#include "RConn.h"

FakeTcp::FakeTcp(uv_stream_t *tcp, const std::string &key, const TcpInfo &info) : INetConn(key), mInfo(info) {
    mTcp = tcp;
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
        FakeTcp *conn = static_cast<FakeTcp *>(stream->data);
        conn->OnTcpError(conn, nread);
    }
    free(buf->base);
}

void FakeTcp::OnTcpError(FakeTcp *conn, int err) {
    LOGE << "conn " << conn->Key() << ", err: " << err;
    mAlive = false;
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

