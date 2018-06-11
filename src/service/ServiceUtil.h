//
// Created by System Administrator on 6/1/18.
//

#ifndef RSOCK_SERVICEUTIL_H
#define RSOCK_SERVICEUTIL_H

#include <string>
#include "IService.h"

class ServiceUtil {
public:
    template<typename T>
    static T GetService(const std::string &name);

//    using SPIterator = std::shared_ptr<IService::IIterator>;
//    static SPIterator NewIterator(IService *service); // failed to compile. why?


    /*
     * If you want to unregister observer during iteration,
     * you must not use this method and call IService.NewIterator yourself, then use Iterator.Remove.
     * Unregister during ServiceUtil::ForEach will throw exception.
     */
    template<class T, class F, typename ...Args>
    static void ForEach(IService *service, const F &f, Args... args);;
};

#include "ServiceUtil.cpp"

#endif //RSOCK_SERVICEUTIL_H
