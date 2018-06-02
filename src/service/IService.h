//
// Created by System Administrator on 5/21/18.
//

#ifndef RSOCK_ISERVICE_H
#define RSOCK_ISERVICE_H

#include <string>
#include <vector>

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

    class IIterator {
    public:
        virtual IObserver *Next() = 0;

        virtual bool HasNext() = 0;
    };

    virtual IIterator *NewIterator();

private:
    class Iterator : public IIterator {
    public:
        explicit Iterator(IService *service);

        IObserver *Next() override;

        bool HasNext() override;

    private:
        IService *mService = nullptr;
        std::vector<IObserver *>::iterator mIterator;
    };

private:
    std::vector<IObserver *> mObserver; // use vector to main register order.
    bool mInited = false;
};


#endif //RSOCK_ISERVICE_H
