//
// Created on 12/17/17.
//

#ifndef RSOCK_PORTMAPPER_H
#define RSOCK_PORTMAPPER_H

#include <map>
#include <string>
#include <vector>

#include "ktype.h"

class PortMapper {
public:
    using PortLists = std::vector<u_int16_t >;

    PortMapper() = default;

    virtual  void SetDstPorts(const PortLists &ports);

    virtual  void SetSrcPorts(const PortLists &ports);

    virtual  void AddDstPort(IUINT16 port);

    virtual  void RemoveDstPort(IUINT16 port);

    virtual  IUINT16 NextDstPort() ;

    virtual  IUINT16 NextSrcPort() ;

    virtual PortLists &GetSrcPortLists();

    virtual PortLists &GetDstPortLists();

//    virtual bool AddOriginPort(const struct sockaddr *addr);

private:
    bool addFn(PortLists &vec, IUINT16 port);
    bool removeFn(PortLists &vec, IUINT16 port);
    IUINT16 nextFn(const PortLists &vec);

    PortLists mSrcPorts;
    PortLists mDstPorts;
};
#endif //RSOCK_PORTMAPPER_H
