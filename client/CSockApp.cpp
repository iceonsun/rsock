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
#include "ClientNetManager.h"
#include "../cap/RCap.h"
#include "../conn/FakeUdp.h"
#include "../bean/RConfig.h"
#include "../src/util/KeyGenerator.h"
#include "CConnErrHandler.h"
#include "../src/singletons/RouteManager.h"
#include "../src/singletons/ConfManager.h"

CSockApp::CSockApp() : ISockApp(false) {}

int CSockApp::Init() {
    if (!mErrorHandler) {
        mErrorHandler = new CConnErrHandler(this);
    }
    int n = ISockApp::Init();
    RouteManager::GetInstance()->AddTargetFront(ConfManager::GetInstance()->Conf().param.targetIp);
    return n;
}

void CSockApp::Close() {
    ISockApp::Close();
    if (mErrorHandler) {
        delete mErrorHandler;
        mErrorHandler = nullptr;
    }
}

RCap *CSockApp::CreateCap(const RConfig &conf) {
    return new RCap(conf.param.dev, conf.param.selfCapIp, {}, conf.param.capPorts,
                    conf.param.targetIp, conf.param.cap_timeout, false);
}

RConn *CSockApp::CreateBtmConn(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) {
    auto *rconn = new RConn(conf.param.hashKey, conf.param.dev, loop, ackPool, false);

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

void CSockApp::addUdpNetConn(CNetGroup *group, IGroup *btm) {
    const auto &conns = btm->GetAllConns();
    for (auto &e: conns) {
        auto *conn = dynamic_cast<IBtmConn *>(e.second);
        auto info = conn->GetInfo();
        auto key = KeyGenerator::KeyForConnInfo(*info);
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

void CSockApp::addTcpNetConn(RConfig &conf, CNetGroup *group, INetManager *netManager) {
    TcpInfo info;
    info.src = conf.param.selfCapInt;
    info.dst = conf.param.targetCapInt;
    auto dstPorts = conf.param.capPorts.GetRawList();

    auto *clientNetManager = dynamic_cast<ClientNetManager *>(netManager);
    assert(clientNetManager);

    for (auto dp : dstPorts) {
        info.sp = 0;
        info.dp = dp;
        auto c = clientNetManager->DialTcpSync(info);
        if (c) {
            if (0 == c->Init()) {
                group->AddNetConn(c);
            } else {
                LOGE << "connection" << c->ToStr() << " init failed";
                c->Close();
                delete c;
            }
        } else {
            LOGE << "Dial tcp " << info.ToStr() << " failed";
        }
    }
}

IConn *CSockApp::CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop, INetManager *netManager) {
    IGroup *btmGroup = dynamic_cast<IGroup *>(btm);
    assert(btmGroup);

    auto group = new CNetGroup(IdBuf2Str(conf.param.id), loop); // test tcp first
    group->SetNetConnErrorHandler(mErrorHandler);

    // add udp conns immediately if enabled
    if (conf.param.type & OM_PIPE_UDP) {
        addUdpNetConn(group, btmGroup);
    }

    // dial tcp conn
    if (conf.param.type & OM_PIPE_TCP) {
        addTcpNetConn(conf, group, netManager);
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

INetManager *CSockApp::CreateNetManager(const RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) {
    return new ClientNetManager(loop, ackPool);
}
