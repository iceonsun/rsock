//
// Created by System Administrator on 1/27/18.
//

#ifndef RSOCK_SERVERNETMANAGER_H
#define RSOCK_SERVERNETMANAGER_H


#include "../net/INetManager.h"
#include "../net/TcpListenPool.h"
#include "../src/util/TcpCmpFn.h"

class TcpAckPool;


/*
 * The uv_tcp_t pool.
 * A valid tcp connection has 2 requirements:
 *      1. The INetManager contains a uv_tcp_t handle;
 *      2. The TcpAckPool has corresponding record of tcp.
 * After RConfig.appKeepAliveSec, the expired record will be cleared if not manually fetched or removed.
 */
class ServerNetManager : public INetManager {
public:
    ServerNetManager(uv_loop_t *loop, const RPortList &ports, const std::string &ip, TcpAckPool *ackPool);

    int Init() override;

    int Close() override;

    virtual void OnNewConnection(uv_tcp_t *tcp);

    virtual bool ContainsTcp(const TcpInfo &info);

    void OnFlush(uint64_t timestamp) override;

    /*
     * return tcp handle and set info.ack, info.seq if both INetManager and TcpAckPool contains the info record
     * if doesn't exist, return null and info is not modified.
     */
    virtual uv_tcp_t *GetTcp(TcpInfo &info);

protected:
    /*
     * Will close conn if add failed: failed to get tcpInfo of conn
     */
    void add2Pool(uv_tcp_t *tcp);

    static IntKeyType HashTcpInfo(const TcpInfo &info);

private:

    struct ConnHelper {
        uv_tcp_t *conn = nullptr;

        uint64_t expireMs = 0;

        TcpInfo info;

        ConnHelper(uv_tcp_t *aConn, uint64_t expireMs, const TcpInfo &info) {
            conn = aConn;
            this->expireMs = expireMs;
            this->info = info;
        }
    };

    const uint64_t POOL_PERSIST_MS = 0; // assigned to TcpAckPool.PersistMs in ctor
    TcpListenPool mListenPool;
    std::map<IntKeyType , ConnHelper> mPool;
//    std::map<TcpInfo, ConnHelper, TcpCmpFn> mPool;
};


#endif //RSOCK_SERVERNETMANAGER_H
