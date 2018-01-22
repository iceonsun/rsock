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
#include "../conn/FakeUdp.h"

CSockApp::CSockApp(uv_loop_t *loop) : ISockApp(false, loop) {}

RCap *CSockApp::CreateCap(RConfig &conf) {
    return new RCap(conf.param.dev, conf.param.selfCapIp, conf.param.selfCapPorts, conf.param.targetCapPorts,
                    conf.param.targetIp, conf.param.interval);
}

IConn *CSockApp::CreateBtmConn(RConfig &conf) {
    RConn *rconn = new RConn(conf.param.hashKey, nullptr);   // todo: add tcp later
    auto ports = conf.param.selfCapPorts.GetRawList();
    auto svr_ports = conf.param.targetCapPorts.GetRawList();
    auto vec = createUdpConns(conf.param.selfCapInt, ports, conf.param.targetCapInt, svr_ports);
    for (auto c: vec) {
        rconn->AddUdpConn(c);
    }
    return rconn;
}

IConn *CSockApp::CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop) {
    IGroup *btmGroup = dynamic_cast<IGroup *>(btm);
    assert(btmGroup);
    
    const auto &conns = btmGroup->GetAllConns();
    auto group = new CNetGroup(IdBuf2Str(conf.param.id), loop);
    for (auto &e: conns) {
        auto *conn = dynamic_cast<INetConn *>(e.second);
        if (conn) {
            auto info = conn->GetInfo();
            auto key = ConnInfo::BuildKey(*info);
            auto fakeudp = new FakeUdp(key, *info);
            group->AddNetConn(fakeudp);
        }
    }

    return new ClientGroup(IdBuf2Str(conf.param.id), conf.param.selfUnPath, conf.param.localUdpIp,
                           conf.param.localUdpPort, loop, group, btm);
}
