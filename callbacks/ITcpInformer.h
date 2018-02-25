//
// Created by System Administrator on 2/9/18.
//

#ifndef RSOCK_TCPINFORMER_H
#define RSOCK_TCPINFORMER_H

#include <set>

#include "ITcpObserver.h"

class ITcpInformer {
public:
    virtual ~ITcpInformer();

    virtual bool Attach(ITcpObserver *observer);

    virtual void Detach(ITcpObserver *observer);

    virtual void Notify(const TcpInfo &info);

private:
    std::set<ITcpObserver *> mObservers;
};


#endif //RSOCK_TCPINFORMER_H
