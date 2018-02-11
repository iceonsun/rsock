//
// Created by System Administrator on 12/26/17.
//

#include <cassert>
#include <plog/Log.h>
#include "SSockApp.h"
#include "ServerGroup.h"
#include "../util/rhash.h"
#include "../conn/RConn.h"
#include "../net/ServerNetManager.h"
#include "../conn/TcpInfo.h"
#include "../cap/RCap.h"

SSockApp::SSockApp(uv_loop_t *loop) : ISockApp(true, loop) {}

RCap *SSockApp::CreateCap(RConfig &conf) {
    return new RCap(conf.param.dev, conf.param.selfCapIp, conf.param.capPorts, {}, "", conf.param.interval, true);
}

RConn *SSockApp::CreateBtmConn(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool, int datalink) {
    // todo: listen tcp later, tcp is listened in ServerNetManager
    RConn *rconn = new RConn(conf.param.hashKey, conf.param.dev, loop, ackPool, datalink, true);
    auto ports = conf.param.capPorts.GetRawList();
    std::vector<uint16_t> zeros(ports.size(), 0);
    auto vec = createUdpConns(conf.param.selfCapInt, ports, 0, zeros);
    for (auto c: vec) {
        rconn->AddUdpConn(c);
    }

    return rconn;
}

IConn *SSockApp::CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop, INetManager *netManager) {
    SA4 target = {0};
    target.sin_family = AF_INET;
    target.sin_port = htons(conf.param.targetPort);
    inet_aton(conf.param.targetIp.c_str(), &target.sin_addr);
    return new ServerGroup(IdBuf2Str(conf.param.id), loop, (const SA *) (&target), btm, netManager);
}

INetManager *SSockApp::CreateNetManager(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) {
    return new ServerNetManager(loop, conf.param.capPorts, conf.param.selfCapIp, ackPool);
}