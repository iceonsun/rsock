//
// Created by System Administrator on 5/21/18.
//

#ifndef RSOCK_ISERVICE_H
#define RSOCK_ISERVICE_H

#include <string>
#include <set>

class IObserver;

class IService {
public:
    virtual ~IService() = default;

    virtual int Init();

    virtual int Close();

    virtual int RegisterObserver(IObserver *observer);

    virtual int UnRegisterObserver(IObserver *observer);

    virtual int ObserverSize() const;

    virtual bool ContainsObserver(IObserver *observer);

private:
    std::set<IObserver *> mObserver;
    bool mInited = false;
};


#endif //RSOCK_ISERVICE_H
