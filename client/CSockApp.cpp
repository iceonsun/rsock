//
// Created by System Administrator on 12/26/17.
//

#include <plog/Log.h>
#include "CSockApp.h"
#include "../conn/RConn.h"
#include "ClientGroup.h"
#include "../conn/BtmUdpConn.h"
#include "CNetGroup.h"
#include "../util/rhash.h"
#include "../net/ClientNetManager.h"
#include "../cap/RCap.h"

CSockApp::CSockApp(uv_loop_t *loop) : ISockApp(false, loop) {}

RCap *CSockApp::CreateCap(RConfig &conf) {
    return new RCap(conf.param.dev, conf.param.selfCapIp, {}, conf.param.targetCapPorts,
                    conf.param.targetIp, conf.param.interval, false);
//    return new RCap(conf.param.dev, conf.param.selfCapIp, conf.param.selfCapPorts, conf.param.targetCapPorts,
//                    conf.param.targetIp, conf.param.interval, false);
}

IConn *CSockApp::CreateBtmConn(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool, int datalink) {
    RConn *rconn = new RConn(conf.param.hashKey, conf.param.dev, loop, ackPool, datalink, false);
    // dial udp conn
//    auto ports = conf.param.selfCapPorts.GetRawList();
//    auto svr_ports = conf.param.targetCapPorts.GetRawList();
//    auto vec = createUdpConns(conf.param.selfCapInt, ports, conf.param.targetCapInt, svr_ports);
//    for (auto c: vec) {
//        rconn->AddUdpConn(c);
//    }

    return rconn;
}

IConn *CSockApp::CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop, INetManager *netManager) {
    IGroup *btmGroup = dynamic_cast<IGroup *>(btm);
    assert(btmGroup);

    auto group = new CNetGroup(IdBuf2Str(conf.param.id), loop); // test tcp first
    auto fn = std::bind(&CSockApp::OnConnErr, this, std::placeholders::_1);
    group->SetNetConnErrCb(fn);
    // add udp conns
//    const auto &conns = btmGroup->GetAllConns();
//    for (auto &e: conns) {
//        auto *conn = dynamic_cast<INetConn *>(e.second);
//        auto info = conn->GetInfo();
//        auto key = ConnInfo::BuildKey(*info);
//        INetConn *c = nullptr;
//        if (conn->IsUdp()) {
//            c = new FakeUdp(key, *info);
//            group->AddNetConn(c);
//        }
//    }

    TcpInfo info;
    info.src = conf.param.selfCapInt;
    info.dst = conf.param.targetCapInt;
    auto srcPorts = conf.param.selfCapPorts.GetRawList();
    auto dstPorts = conf.param.targetCapPorts.GetRawList();
    const int SIZE = std::min(srcPorts.size(), dstPorts.size());
    auto manager = GetNetManager();

    auto *clientNetManager = dynamic_cast<ClientNetManager *>(manager);
    assert(clientNetManager);

    for (int i = 0; i < SIZE; i++) {
        info.sp = 0;
        info.dp = dstPorts[i];
        auto c = clientNetManager->DialTcpSync(info);
        if (c) {
            if (0 == c->Init()) {
                group->AddNetConn(c);
            } else {
                LOGE << "connection" << c->Key() << " init failed";
                c->Close();
                delete c;
            }
        } else {
            LOGE << "Dial tcp " << info.ToStr() << " failed";
        }
    }

    return new ClientGroup(IdBuf2Str(conf.param.id), conf.param.selfUnPath, conf.param.localUdpIp,
                           conf.param.localUdpPort, loop, group, btm);
}

INetManager *CSockApp::CreateNetManager(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) {
    return new ClientNetManager(loop, ackPool);
}

void CSockApp::OnConnErr(const ConnInfo &info) {
    if (!info.IsUdp()) {
        auto manager = GetNetManager();
        auto *clientNetManager = dynamic_cast<ClientNetManager *>(manager);
        assert(clientNetManager);

        LOGE << "conn " << info.ToStr() << ", err, reconnect it";
        auto cb = std::bind(&CSockApp::TcpDialAsyncCb, this, std::placeholders::_1, std::placeholders::_2);
        clientNetManager->DialTcpAsync(info, cb);
    }
}

void CSockApp::TcpDialAsyncCb(INetConn *conn, const ConnInfo &info) {
    auto *clientGroup = dynamic_cast<ClientGroup *>(GetBridgeConn());
    assert(clientGroup);
    LOGD << "reconnecting conn " << info.ToStr() << " succeeds.";
    if (conn) {
        clientGroup->GetNetGroup()->AddNetConn(conn);
    }
}
