//
// Created by System Administrator on 6/9/18.
//

#ifndef RSOCK_APPNETOBSERVER_H
#define RSOCK_APPNETOBSERVER_H


#include "../service/INetObserver.h"

class ISockApp;

class AppNetObserver: public INetObserver {
public:
    explicit AppNetObserver(ISockApp *app);

    void OnTcpFinOrRst(const TcpInfo &info) override;

    int Init() override;

    int Close() override;

private:
    ISockApp *mApp = nullptr;
};


#endif //RSOCK_APPNETOBSERVER_H
