//
// Created by System Administrator on 1/27/18.
//

#include <plog/Log.h>
#include "ServerNetManager.h"
#include "../conn/TcpInfo.h"
#include "../util/rsutil.h"
#include "NetUtil.h"
#include "../conn/FakeTcp.h"

using namespace std::placeholders;

ServerNetManager::ServerNetManager(uv_loop_t *loop, const RPortList &ports, const std::string &ip, TcpAckPool *ackPool)
        : INetManager(loop, ackPool), mListenPool(ports, ip, loop) {
}

int ServerNetManager::Init() {
    int nret = INetManager::Init();
    if (nret) {
        return nret;
    }
    auto fn = std::bind(&ServerNetManager::OnNewConnection, this, _1);
    mListenPool.SetNewConnCb(fn);
    return mListenPool.Init();
}

void ServerNetManager::OnNewConnection(uv_tcp_t *tcp) {
    TcpInfo info;
    if (0 == GetTcpInfo(info, tcp)) {
        auto c = NetUtil::CreateTcpConn(tcp);
        auto key = c->Key();
        if (add2PoolAutoClose(c)) {
            LOGD << "no tcp record in pool for conn " << key;
        }
    } else {    // get information failed
        uv_close(reinterpret_cast<uv_handle_t *>(tcp), close_cb);
    }
}

void ServerNetManager::Close() {
    INetManager::Close();
    mListenPool.Close();
    mListenPool.SetNewConnCb(nullptr);
}
