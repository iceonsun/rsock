//
// Created by System Administrator on 12/26/17.
//

#include "CSockApp.h"
#include "CRawConn.h"

CSockApp::CSockApp(uv_loop_t *loop) : ISockApp(false, loop) {}

RCap *CSockApp::CreateCap(RConfig &conf) {
    return new RCap(conf.param.dev, conf.param.selfCapIp, conf.param.selfCapPorts, conf.param.targetCapPorts, conf.param.targetIp, conf.param.interval);
}

IRawConn *CSockApp::CreateBtmConn(RConfig &conf, uv_loop_t *loop, int datalink, int conn_type) {
    return new CRawConn(conf.param.dev, conf.param.selfCapInt, loop, conf.param.hashKey, conf.param.targetCapInt, datalink,
                        conn_type);
}

IConn *CSockApp::CreateBridgeConn(RConfig &conf, IRawConn *btm, uv_loop_t *loop) {
    return new ClientConn(conf.param.id, conf.param.selfUnPath, conf.param.localUdpIp, conf.param.localUdpPort, conf.param.selfCapPorts, conf.param.targetCapPorts,
                          loop, btm, conf.param.targetCapInt, conf.param.type);
}
