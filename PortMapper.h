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
    using PortLists = std::vector<IUINT16 >;

    PortMapper() = default;

    virtual  void SetDstPorts(const std::vector<IUINT16> &ports);

    virtual  void SetSrcPorts(const std::vector<IUINT16> &ports);

    virtual  void AddDstPort(IUINT16 port);

    virtual  void RemoveDstPort(IUINT16 port);

    virtual  IUINT16 NextDstPort() ;

    virtual  IUINT16 NextSrcPort() ;

//    virtual PortLists &GetSrcPortLists();

//    virtual PortLists &GetDstPortLists();

//    virtual bool AddOriginPort(const struct sockaddr *addr);

private:
    bool addFn(std::vector<IUINT16> &vec, IUINT16 port);
    bool removeFn(std::vector<IUINT16> &vec, IUINT16 port);
    IUINT16 nextFn(const std::vector<IUINT16> &vec);

    std::vector<IUINT16> mSrcPorts;
    std::vector<IUINT16> mDstPorts;
};
#endif //RSOCK_PORTMAPPER_H
