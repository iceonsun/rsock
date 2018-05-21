//
// Created by System Administrator on 6/9/18.
//

#include <cassert>
#include <plog/Log.h>
#include "CConnErrHandler.h"
#include "CSockApp.h"
#include "ClientNetManager.h"
#include "../bean/ConnInfo.h"
#include "ClientGroup.h"
#include "../conn/INetGroup.h"

CConnErrHandler::CConnErrHandler(CSockApp *app) {
    mApp = app;
}

void CConnErrHandler::OnNetConnErr(const ConnInfo &info, int errCode) {
    if (!mApp->IsClosing()) {
        auto manager = mApp->GetNetManager();
        auto *clientNetManager = dynamic_cast<ClientNetManager *>(manager);
        assert(clientNetManager);
        LOGE << "conn " << info.ToStr() << ", err, reconnect it";
        if (!info.IsUdp()) {
            auto cb = std::bind(&CConnErrHandler::TcpDialAsyncCb, this, std::placeholders::_1, std::placeholders::_2);
            ConnInfo newInfo = info;
            newInfo.sp = 0;     // sp = 0;
            clientNetManager->DialTcpAsync(newInfo, cb);
        } else {
            // todo: dial udp. succeeds immediately
        }
    }
}

void CConnErrHandler::TcpDialAsyncCb(INetConn *conn, const ConnInfo &info) {
    auto *clientGroup = dynamic_cast<ClientGroup *>(mApp->GetBridgeConn());
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

int CConnErrHandler::Close() {
    mApp = nullptr;
    return 0;   // remove all callback in clientNetManager?
}

