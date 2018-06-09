//
// Created by System Administrator on 6/9/18.
//

#ifndef RSOCK_CLIENTNETOBSERVER_H
#define RSOCK_CLIENTNETOBSERVER_H


#include "../src/service/INetObserver.h"

class ClientGroup;

class ClientNetObserver : public INetObserver {
public:
    explicit ClientNetObserver(ClientGroup *group);

    void OnTcpFinOrRst(const TcpInfo &info) override;

    int Init() override;

    int Close() override;

private:
    ClientGroup *mGroup = nullptr;
};


#endif //RSOCK_CLIENTNETOBSERVER_H
