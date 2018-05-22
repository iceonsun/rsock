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
#include "../conn/FakeUdp.h"

CSockApp::CSockApp() : ISockApp(false) {}

RCap *CSockApp::CreateCap(RConfig &conf) {
    return new RCap(conf.param.dev, conf.param.selfCapIp, {}, conf.param.capPorts,
                    conf.param.targetIp, conf.param.cap_timeout, false);
}

RConn *CSockApp::CreateBtmConn(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool, int datalink) {
    auto *rconn = new RConn(conf.param.hashKey, conf.param.dev, loop, ackPool, datalink, false);

    // add udp btm conn to RConn if udp enabled
    if (conf.param.type & OM_PIPE_UDP) {
        auto svr_ports = conf.param.capPorts.GetRawList();
        std::vector<uint16_t> ports(svr_ports.size(), 0);
        auto vec = bindUdpConns(conf.param.selfCapInt, ports, conf.param.targetCapInt, svr_ports);
        for (auto c: vec) {
            rconn->AddUdpConn(c);
        }
    }

    return rconn;
}

IConn *CSockApp::CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop, INetManager *netManager) {
    IGroup *btmGroup = dynamic_cast<IGroup *>(btm);
    assert(btmGroup);

    auto group = new CNetGroup(IdBuf2Str(conf.param.id), loop); // test tcp first
    auto fn = std::bind(&CSockApp::OnConnErr, this, std::placeholders::_1);
    group->SetNetConnErrCb(fn);

    // add udp conns immediately if enabled
    if (conf.param.type & OM_PIPE_UDP) {
        const auto &conns = btmGroup->GetAllConns();
        for (auto &e: conns) {
            auto *conn = dynamic_cast<IBtmConn *>(e.second);
            auto info = conn->GetInfo();
            auto key = INetConn::BuildKey(*info);
            INetConn *c = nullptr;
            if (conn->IsUdp()) {
                c = new FakeUdp(key, *info);
                if (c->Init()) {
                    c->Close();
                    delete c;
                    continue;
                }
                group->AddNetConn(c);
            }
        }
    }

    // dial tcp conn
    if (conf.param.type & OM_PIPE_TCP) {
        TcpInfo info;
        info.src = conf.param.selfCapInt;
        info.dst = conf.param.targetCapInt;
        auto dstPorts = conf.param.capPorts.GetRawList();

        auto manager = GetNetManager();
        auto *clientNetManager = dynamic_cast<ClientNetManager *>(manager);
        assert(clientNetManager);

        for (auto dp : dstPorts) {
            info.sp = 0;
            info.dp = dp;
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
    }

    if (group->GetAllConns().empty()) {
        LOGE << "dial failed";
        group->Close();
        delete group;
        return nullptr;
    }
    return new ClientGroup(IdBuf2Str(conf.param.id), conf.param.selfUnPath, conf.param.localUdpIp,
                           conf.param.localUdpPort, loop, group, btm);
}

INetManager *CSockApp::CreateNetManager(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) {
    return new ClientNetManager(loop, ackPool);
}

// todo: add udp reconnect
void CSockApp::OnConnErr(const ConnInfo &info) {
    if (!IsClosing()) {
        auto manager = GetNetManager();
        auto *clientNetManager = dynamic_cast<ClientNetManager *>(manager);
        assert(clientNetManager);
        LOGE << "conn " << info.ToStr() << ", err, reconnect it";
        if (!info.IsUdp()) {
            auto cb = std::bind(&CSockApp::TcpDialAsyncCb, this, std::placeholders::_1, std::placeholders::_2);
            ConnInfo newInfo = info;
            newInfo.sp = 0;     // sp = 0;
            clientNetManager->DialTcpAsync(newInfo, cb);
        } else {
            // todo: dial udp. succeeds immediately
        }

    }
}

void CSockApp::TcpDialAsyncCb(INetConn *conn, const ConnInfo &info) {
    auto *clientGroup = dynamic_cast<ClientGroup *>(GetBridgeConn());
    assert(clientGroup);
    LOGD << "reconnecting conn " << info.ToStr() << ((conn != nullptr) ? " succeeds." : "failed");
    if (conn) {
        if (conn->Init()) {
            LOGE << "conn " << conn->ToStr() << " init failed";
            conn->Close();
            delete conn;
            return;
        }
        clientGroup->GetNetGroup()->AddNetConn(conn);
    }
}