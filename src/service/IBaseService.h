//
// Created by System Administrator on 6/8/18.
//

#ifndef RSOCK_IBASESERVICE_H
#define RSOCK_IBASESERVICE_H


#include "IService.h"

template<typename T>
class IBaseService : public IService {
public:
    virtual int RegisterObserver(T *observer);

    virtual int UnRegisterObserver(T *observer);

private:
    using IService::RegisterObserver;
    using IService::UnRegisterObserver;
};

template<typename T>
int IBaseService<T>::RegisterObserver(T *observer) {
    //return IService::RegisterObserver(dynamic_cast<IObserver *>(observer));
    return IService::RegisterObserver(observer);
}

template<typename T>
int IBaseService<T>::UnRegisterObserver(T *observer) {
    //return IService::UnRegisterObserver(dynamic_cast<IObserver *>(observer));
    return IService::UnRegisterObserver(observer);
}


#endif //RSOCK_IBASESERVICE_H
