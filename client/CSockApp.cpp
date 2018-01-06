//
// Created by System Administrator on 12/26/17.
//

#include "CSockApp.h"
#include "CRawConn.h"
#include "../tcp/SockMon.h"
#include "../tcp/TcpConnector.h"

CSockApp::CSockApp(uv_loop_t *loop) : ISockApp(false, loop) {}

RCap *CSockApp::CreateCap(RConfig &conf) {
    return new RCap(conf.param.dev, conf.param.selfCapIp, conf.param.selfCapPorts, conf.param.targetCapPorts,
                    conf.param.targetIp, conf.param.interval);
}

IRawConn *CSockApp::CreateBtmConn(RConfig &conf, uv_loop_t *loop, int datalink, int conn_type) {
    return new CRawConn(conf.param.dev, conf.param.selfCapInt, loop, conf.param.hashKey, conf.param.targetCapInt,
                        datalink, conn_type);
}

IConn *CSockApp::CreateBridgeConn(RConfig &conf, IRawConn *btm, uv_loop_t *loop, SockMon *mon) {
    return new ClientConn(conf.param.id, conf.param.selfUnPath, conf.param.localUdpIp, conf.param.selfCapInt,
                          conf.param.localUdpPort, conf.param.selfCapPorts, conf.param.targetCapPorts, loop, mon, btm,
                          conf.param.targetCapInt, conf.param.type);
}

SockMon *CSockApp::InitSockMon(uv_loop_t *loop, const RConfig &conf) {
    PortMapper::PortPairList lists;
    PortMapper::BuildPairs(conf.param.selfCapPorts, conf.param.targetCapPorts, lists);

    std::vector<int> socks;
    int nret = TcpConnector::SyncConnect(conf.param.selfCapIp, conf.param.targetIp, lists, socks);
    if (nret) {
        return nullptr;
    }
    auto mon = new SockMon(loop, nullptr);
    mon->Init();
    for (auto s: socks) {
        if (!mon->Add(s)) {
            mon->Close();
            delete mon;
            return nullptr;
        }
    }
    return mon;
}
