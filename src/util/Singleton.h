//
// Created by System Administrator on 6/1/18.
//

#ifndef RSOCK_SINGLETON_H
#define RSOCK_SINGLETON_H

#include <mutex>
#include <cassert>
#include "ICloseable.h"

template<typename T>
class Singleton {
public:
    static T *GetInstance();

    static int DestroyInstance();

    virtual ~Singleton();

protected:
    Singleton();

private:
    static std::mutex sMutex;
    static T *sInstance;
    static bool sDestroyed;
};


template<typename T>
std::mutex Singleton<T>::sMutex;

template<typename T>
T *Singleton<T>::sInstance = nullptr;


template<typename T>
bool Singleton<T>::sDestroyed;

#include "Singleton.cpp"

#endif //RSOCK_SINGLETON_H
