//
// Created by System Administrator on 1/19/18.
//

#ifndef RSOCK_IAPPCONN_H
#define RSOCK_IAPPCONN_H

#include "IGroup.h"
#include "../callbacks/ITcpObserver.h"
#include "../bean/EncHead.h"
#include "../callbacks/NetConnKeepAlive.h"
#include "../callbacks/IReset.h"

class INetGroup;

class INetConn;

struct ConnInfo;

class IAppGroup : public IGroup, public ITcpObserver {
public:
    using IntKeyType = uint32_t;

    IAppGroup(const std::string &groupId, INetGroup *fakeNetGroup, IConn *btm, bool activeKeepAlive, const std::string &printableStr = "");

    int Init() override;

    void Close() override;

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    int Input(ssize_t nread, const rbuf_t &rbuf) override;

    INetGroup *GetNetGroup() const { return mFakeNetGroup; }

    void Flush(uint64_t now) override;

    INetGroup *NetGroup() { return mFakeNetGroup; }

    bool OnTcpFinOrRst(const TcpInfo &info) override;

//    bool OnUdpRst(const ConnInfo &info) override;

    bool Alive() override;

    virtual int SendConvRst(uint32_t conv);

    const std::string ToStr() override;

protected:
    virtual int sendNetConnRst(const ConnInfo &src, IntKeyType key);

    virtual int onPeerNetConnRst(const ConnInfo &src, uint32_t key);

    virtual int onPeerConvRst(const ConnInfo &src, uint32_t rstConv);

    virtual bool onSelfNetConnRst(const ConnInfo &info);

    virtual int doSendCmd(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf);

    virtual int onNetconnDead(uint32_t key);

protected:
    class NetConnKeepAliveHelper : public NetConnKeepAlive::INetConnAliveHelper {
    public:
        explicit NetConnKeepAliveHelper(IAppGroup *group, uv_loop_t *loop, bool active = true);

        int OnSendResponse(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

        int OnRecvResponse(IntKeyType connKey) override;

        int OnSendRequest(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

        INetConn *ConnOfIntKey(IntKeyType connKey) override;

        int SendNetConnRst(const ConnInfo &src, IntKeyType connKey) override;

        void Close() override;

        INetConnKeepAlive *GetIKeepAlive() const override;

    private:
        void onFlush();

    private:
        void setupTimer(uv_loop_t *loop);

        static void timer_cb(uv_timer_t *timer);

    private:
        const int MAX_RETRY = 3;
        const uint32_t FLUSH_INTERVAL = 5000;  // every 5sec
        const uint32_t FIRST_FLUSH_DELAY = 5000;   // on app start
        IAppGroup *mAppGroup = nullptr;
        uv_timer_t *mFlushTimer = nullptr;
        std::map<IntKeyType, int> mReqMap;
        INetConnKeepAlive *mKeepAlive = nullptr;
    };

    class ResetHelper : public IReset::IRestHelper {
    public:
        explicit ResetHelper(IAppGroup *appGroup);

        void Close() override;

        int OnSendNetConnReset(uint8_t cmd, const ConnInfo &src, ssize_t nread, const rbuf_t &rbuf) override;

        int OnSendConvRst(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

        int OnRecvNetconnRst(const ConnInfo &src, IntKeyType key) override;

        int OnRecvConvRst(const ConnInfo &src, uint32_t conv) override;

        IReset *GetReset() override;

    private:
        IAppGroup *mAppGroup = nullptr;
        IReset *mReset = nullptr;
    };

protected:
    EncHead mHead;

private:
    bool mActive = true;
    std::string mPrintableStr;
    ResetHelper *mResetHelper = nullptr;
    NetConnKeepAliveHelper::INetConnAliveHelper *mKeepAliveHelper = nullptr;
    INetGroup *mFakeNetGroup = nullptr;
};


#endif //RSOCK_IAPPCONN_H
