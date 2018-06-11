//
// Created by System Administrator on 6/10/18.
//

#ifndef RSOCK_TCPINFOCMP_H
#define RSOCK_TCPINFOCMP_H

#include "../../bean/TcpInfo.h"

struct TcpCmpFn {
    inline bool operator()(const TcpInfo &info1, const TcpInfo &info2) const {
        return (info1.src < info2.src) || (info1.sp < info2.sp) || (info1.dst < info2.dst) || (info1.dp < info2.dp);
    }
};


#endif //RSOCK_TCPINFOCMP_H
