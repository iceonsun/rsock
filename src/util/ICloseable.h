//
// Created by System Administrator on 6/2/18.
//

#ifndef RSOCK_ICLOSEABLE_H
#define RSOCK_ICLOSEABLE_H


class ICloseable {
public:
    virtual ~ICloseable() = default;

    virtual int Close() = 0;
};


#endif //RSOCK_ICLOSEABLE_H
