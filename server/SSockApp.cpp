//
// Created by System Administrator on 12/26/17.
//

#include "SSockApp.h"
#include "../RConfig.h"
#include "SRawConn.h"
#include "../RConfig.h"

SSockApp::SSockApp(uv_loop_t *loop) : ISockApp(true, loop) {}

RCap *SSockApp::CreateCap(RConfig &conf) {
    return new RCap(conf.param.dev, conf.param.selfCapIp, conf.param.selfCapPorts, {}, "", conf.param.interval);
}

IRawConn *SSockApp::CreateBtmConn(RConfig &conf, uv_loop_t *loop, int datalink, int conn_type) {
    return new SRawConn(conf.param.dev, conf.param.selfCapInt, loop, conf.param.hashKey, datalink, conn_type);
}

IConn *SSockApp::CreateBridgeConn(RConfig &conf, IRawConn *btm, uv_loop_t *loop) {
    struct sockaddr_in target = {0};
    target.sin_family = AF_INET;
    target.sin_port = htons(conf.param.targetPort);
    inet_aton(conf.param.targetIp.c_str(), &target.sin_addr);
    return new ServerGroupConn(conf.param.id, loop, btm, reinterpret_cast<const sockaddr *>(&target));
}
