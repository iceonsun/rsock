//
// Created by System Administrator on 1/16/18.
//

#include <cassert>
#include <plog/Log.h>
#include "FakeTcp.h"
#include "RConn.h"
#include "../util/rsutil.h"
#include "../src/util/KeyGenerator.h"
#include "../src/util/TcpUtil.h"

// If send 1 bytes for a specific interval to check if alive. This will severely slow down speed
FakeTcp::FakeTcp(uv_tcp_t *tcp, IntKeyType key, const TcpInfo &info) : INetConn(key), mInfo(info) {
    assert(tcp);
    mTcp = (uv_stream_t *) tcp;

    TcpInfo anInfo;
    GetTcpInfo(anInfo, reinterpret_cast<uv_tcp_t *>(mTcp));
    assert(anInfo.src == info.src && anInfo.dst == info.dst && anInfo.sp == info.sp && anInfo.dp == info.dp);
}

int FakeTcp::Init() {
    INetConn::Init();
    assert(mTcp);

    TcpUtil::SetISN(this, mInfo);
    TcpUtil::SetAckISN(this, mInfo);

    mTcp->data = this;
    return uv_read_start(mTcp, alloc_buf, read_cb);
}

int FakeTcp::Close() {
    INetConn::Close();
    if (mTcp) {
        uv_close(reinterpret_cast<uv_handle_t *>(mTcp), close_cb);
        mTcp = nullptr;
    }
    return 0;
}

int FakeTcp::Output(ssize_t nread, const rbuf_t &rbuf) {
    int n = INetConn::Output(nread, rbuf);
    if (n >= 0) {
        // todo: when add aes or encrpytion, here need to change
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

    if (n >= 0) {
        if (mInfo.ack < info->seq) {
            mInfo.UpdateAck(info->seq);
        }
    }

    return n;
}

void FakeTcp::read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    // this may never receive any packets.
    if (nread < 0 && nread != UV_ECANCELED) {
        FakeTcp *conn = static_cast<FakeTcp *>(stream->data);
        LOGE << "conn " << conn->ToStr() << " err: " << uv_strerror(nread);
        conn->onTcpError(conn, nread);
    }
    free(buf->base);
}

void FakeTcp::onTcpError(FakeTcp *conn, int err) {
    NotifyErr(ERR_FIN_RST);
}

void FakeTcp::SetISN(uint32_t isn) {
    mInfo.UpdateSeq(isn);
}

void FakeTcp::SetAckISN(uint32_t isn) {
    mInfo.UpdateAck(isn);
}

// The server will send keepalive now, so the alive state is determined by keepalive and tcp rst/fin if received any
//bool FakeTcp::Alive() {
//    return mAlive;
//}

ConnInfo *FakeTcp::GetInfo() {
    return &mInfo;
}

bool FakeTcp::IsUdp() {
    return false;
}
