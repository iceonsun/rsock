//
// Created by System Administrator on 1/4/18.
//

#include <cassert>
#include <ctime>

#include <thread>

#include <arpa/inet.h>

#include <plog/Log.h>
#include "TcpConnector.h"
#include "../util/TextUtils.h"
#include "../util/rsutil.h"


TcpConnector::TcpConnector(RPortList &ports, const std::string &ip)
        : TcpConnector(ports.GetRawList(), ports.GetRawList().size(), ip) {
}

TcpConnector::TcpConnector(int n, const std::string &ip)
        : TcpConnector(RPortList::PortList(n, 0), n, ip) {
}

TcpConnector::TcpConnector(const RPortList::PortList &ports, int n, const std::string &ip)
        : mIp(ip), mSocks(n, 0) {
    mPorts = ports;
    mNumConn = n;

    checkValidation();
}

int TcpConnector::Init() {
    int nret = 0;

    struct in_addr addr = {0};
    nret = inet_aton(mIp.c_str(), &addr);
    if (1 != nret) {
        LOGE << "ip " << mIp << " is a invalid ipv4 address.";
        return -1;
    }
    if (mSyncConnect) {
        nret = syncInit(addr.s_addr);
    } else {
        nret = asyncInit(addr.s_addr);
    }
    if (nret) {
        return nret;
    }

    return nret;
}

int TcpConnector::syncInit(in_addr_t inAddr) {
    return syncConnect(mPorts, inAddr, mSocks);
}

int TcpConnector::syncConnect(RPortList::PortList &ports, in_addr_t inAddr, std::vector<int> &socks) {
    int nret = 0;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inAddr;
    int i = 0;
    socklen_t socklen = sizeof(struct sockaddr_in);
    do {
        for (i = 0; i < ports.size(); i++) {
            socks[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (-1 == socks[i]) {
                LOGE << "error creating socket: " << strerror(errno);
            }
            if (ports[i] > 0) {
                addr.sin_port = htons(ports[i]);
                nret = bind(socks[i], reinterpret_cast<const sockaddr *>(&addr), socklen);
                if (nret) {
                    LOGE << "failed to bind  " << InAddr2Ip(addr.sin_addr) << ":" << ports[i] << ", err: "
                         << strerror(errno);
                    break;
                }
            } else {
                addr.sin_port = 0;
            }
            nret = connect(socks[i], reinterpret_cast<const sockaddr *>(&addr), socklen);
            if (nret) {
                LOGE << "failed to connect " << InAddr2Ip(addr.sin_addr) << ":" << ports[i] << ", err: "
                     << strerror(errno);
                break;
            }
        }
    } while (false);
    if (nret) {
        for (int j = 0; j < i; j++) {
            close(socks[j]);
            socks[j] = -1;
        }
    }
    return nret;
}

int TcpConnector::asyncInit(in_addr_t inAddr) {
    std::vector<std::thread> threads(mNumConn);
    std::vector<int> errnos(mNumConn, 0);

    int nret = 0;
    long time1 = time(NULL);
    for (int i = 0; i < mNumConn; i++) {
        threads[i] = std::thread(asyncConnect, mPorts[i], inAddr, std::ref(mSocks[i]), std::ref(errnos[i]));
    }

    long time2 = time(NULL);
    LOGD << "async connect costs " << (time2 - time1) << " seconds";

    for (int i = 0; i < mNumConn; i++) {
        threads[i].join();
        if (mSocks[i] < 0) {
            LOGE << "error to connect: " << strerror(errnos[i]);
            nret = -1;
        }
    }

    if (nret) {
        LOGE << "failed to connect to server.";
        for (int i = 0; i < mNumConn; i++) {    // close sock if success
            if (mSocks[i] > 0) {
                close(mSocks[i]);
                mSocks[i] = -1;
            }
        }
    } else {
        LOGD << "sucessfully connect on ports: " << TextUtils::Vector2String<uint16_t>(mPorts);
    }
    return nret;
}

int TcpConnector::asyncConnect(uint16_t port, in_addr_t inAddr, int &sock, int &err) {
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inAddr;

    int nret = 0;
    do {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (-1 == sock) {
            nret = -1;
            break;
        }

        socklen_t socklen = sizeof(struct sockaddr_in);
        if (port > 0) {
            addr.sin_port = htons(port);
            nret = bind(sock, reinterpret_cast<const sockaddr *>(&addr), socklen);
            if (nret) {
                break;
            }
        }
        nret = connect(sock, reinterpret_cast<const sockaddr *>(&addr), socklen);
    } while (false);

    if (nret) {
        sock = nret;
        err = errno;
    }

    return 0;
}

void TcpConnector::checkValidation() {
    assert(mNumConn > 0);
    assert(mPorts.size() == mNumConn);
}

int TcpConnector::Close() {
    for (auto sock: mSocks) {
        if (sock > 0) {
            close(sock);
        }
    }
    mSocks.clear();
    return 0;
}

int TcpConnector::Flush() {
    return 0;
}

int TcpConnector::SyncConnect(const std::string &selfIp, const std::string &targetIp,
                              PortMapper::PortPairList &portPairs, std::vector<int> &socks) {
    int nret = 0;
    struct sockaddr_in selfAddr = {0};
    selfAddr.sin_family = AF_INET;
    nret = inet_aton(selfIp.c_str(), &selfAddr.sin_addr);
    assert(nret == 1);

    struct sockaddr_in targetAddr = {0};
    targetAddr.sin_family = AF_INET;
    nret = inet_aton(targetIp.c_str(), &targetAddr.sin_addr);
    assert(nret == 1);

    int i = 0;
    socks.resize(portPairs.size(), 0);
    socklen_t socklen = sizeof(struct sockaddr_in);

    struct sockaddr_in opt = {0};

    do {
        for (i = 0; i < portPairs.size(); i++) {
            socks[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (-1 == socks[i]) {
                LOGE << "error creating socket: " << strerror(errno);
            }
            auto &p = portPairs[i];
            if (p.source > 0) {
                selfAddr.sin_port = htons(p.source);
                nret = bind(socks[i], reinterpret_cast<const sockaddr *>(&selfAddr), socklen);
                if (nret) {
                    LOGE << "failed to bind  " << InAddr2Ip(selfAddr.sin_addr) << ":" << p.source << ", err: "
                         << strerror(errno);
                    break;
                }
            } else {
                selfAddr.sin_port = 0;
            }
            if (p.dest > 0) {
                targetAddr.sin_port = htons(p.dest);
            } else {
                p.dest = 0;
            }

            socklen = sizeof(targetAddr);
            nret = connect(socks[i], reinterpret_cast<const sockaddr *>(&targetAddr), socklen);
            if (nret) {
                LOGE << "failed to connect " << Addr2Str(reinterpret_cast<const sockaddr *>(&targetAddr)) << ", err: "
                     << strerror(errno);
                break;
            }
            if (p.source <= 0) {
                socklen = sizeof(opt);
                nret = getsockname(socks[i], reinterpret_cast<sockaddr *>(&opt), &socklen);
                if (nret) {
                    LOGE << "getsockname failed: " << strerror(errno);
                    break;
                }
                p.source = ntohs(opt.sin_port);
            }
        }
    } while (false);
    if (nret) {
        for (int j = 0; j < i; j++) {
            close(socks[j]);
            socks[j] = -1;
        }
        socks.clear();
    }
    return nret;
}
