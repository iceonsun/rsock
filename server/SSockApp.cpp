//
// Created by System Administrator on 12/26/17.
//

#include <cassert>
#include <plog/Log.h>
#include "SSockApp.h"
#include "ServerGroup.h"
#include "../util/rhash.h"
#include "../conn/RConn.h"

SSockApp::SSockApp(uv_loop_t *loop) : ISockApp(true, loop) {}

RCap *SSockApp::CreateCap(RConfig &conf) {
    return new RCap(conf.param.dev, conf.param.selfCapIp, conf.param.selfCapPorts, {}, "", conf.param.interval);
}

IConn * SSockApp::CreateBtmConn(RConfig &conf) {
    RConn *rconn = new RConn(conf.param.hashKey, nullptr);   // todo: add tcp later
    auto ports = conf.param.selfCapPorts.GetRawList();
    std::vector<uint16_t> zeros(ports.size(), 0);
    auto vec = createUdpConns(conf.param.selfCapInt, ports, 0, zeros);
    for (auto c: vec) {
        rconn->AddUdpConn(c);
    }
    return rconn;
}

IConn *SSockApp::CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop) {
    SA4 target = {0};
    target.sin_family = AF_INET;
    target.sin_port = htons(conf.param.targetPort);
    inet_aton(conf.param.targetIp.c_str(), &target.sin_addr);
    return new ServerGroup(IdBuf2Str(conf.param.id), loop, (const SA *) (&target), btm);
}