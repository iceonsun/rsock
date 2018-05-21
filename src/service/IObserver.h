//
// Created by System Administrator on 5/23/18.
//

#ifndef RSOCK_IOBSERVER_H
#define RSOCK_IOBSERVER_H

class IObserver {
public:

    virtual ~IObserver() = default;

    virtual int Init() = 0;

    /*
     * Subclass should override this method and call Service.Unregister(this).
     */
    virtual int Close() = 0;
};

#endif //RSOCK_IOBSERVER_H
