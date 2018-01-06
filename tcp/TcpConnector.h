//
// Created by System Administrator on 1/4/18.
//

#ifndef RSOCK_TCPCONNECTOR_H
#define RSOCK_TCPCONNECTOR_H


#include "../util/RPortList.h"
#include "../PortMapper.h"

class TcpConnector {
public:
    TcpConnector(RPortList &ports, const std::string &ip);

    TcpConnector(int n, const std::string &ip);

    // this may take a while
    virtual int Init();

    virtual int Close();

    virtual int Flush();

    static int SyncConnect(const std::string &selfIp, const std::string &targetIp,
                           PortMapper::PortPairList &portPairs, std::vector<int> &socks);

protected:
    static int asyncConnect(uint16_t port, in_addr_t inAddr, int &sock, int &err);

    static int syncConnect(RPortList::PortList &ports, in_addr_t inAddr, std::vector<int> &socks);

private:
    TcpConnector(const RPortList::PortList &ports, int n, const std::string &ip);

    virtual int syncInit(in_addr_t inAddr);

    int asyncInit(in_addr_t inAddr);

    void checkValidation();

private:
    int mNumConn;
    RPortList::PortList mPorts;
    const std::string mIp;
    std::vector<int> mSocks;
    bool mSyncConnect = true;

};

#endif //RSOCK_TCPCONNECTOR_H
