//
// Created by System Administrator on 5/21/18.
//

#ifndef RSOCK_ISERVICE_H
#define RSOCK_ISERVICE_H

#include <vector>
#include <atomic>

class IObserver;

class IService {
public:
    virtual ~IService() = default;

    IService();

    virtual int Init();

    virtual int Close();

    /*
     * May throw std::logic_error("ConcurrentModificationException");
     */
    virtual int RegisterObserver(IObserver *observer);

    /*
     * May throw std::logic_error("ConcurrentModificationException");
     */
    virtual int UnRegisterObserver(IObserver *observer);

    virtual int ObserverSize() const;

    virtual bool ContainsObserver(IObserver *observer);

    class IIterator {
    public:
        explicit IIterator(IService *service);

        virtual ~IIterator();

        virtual IObserver *Next() = 0;

        virtual bool HasNext() = 0;

        virtual void Remove() = 0;

//    protected:
//        virtual IObserver *moveToNext() = 0;
    private:
        IService *mService = nullptr;
        bool mInited = false;
    };

    virtual IIterator *NewIterator();

private:
    class Iterator : public IIterator {
    public:
        explicit Iterator(IService *service);

        IObserver *Next() override;

        bool HasNext() override;

        void Remove() override;

    private:
        IService *mService = nullptr;
        std::vector<IObserver *>::iterator mIterator;
    };

private:
    std::vector<IObserver *> mObserver; // use vector to main register order.
    bool mInited = false;
    std::atomic<int> mVisitCount;
};


#endif //RSOCK_ISERVICE_H
