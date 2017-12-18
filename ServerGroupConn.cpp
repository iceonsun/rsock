//
// Created on 12/17/17.
//

#include "ServerGroupConn.h"

ServerGroupConn::ServerGroupConn(const char *groupId, IConn *btm) : IGroupConn(groupId, btm) {

}

void ServerGroupConn::AddConn(IConn *conn, const struct sockaddr *addr) {
    IGroupConn::AddConn(conn, addr);

}

int ServerGroupConn::Input(ssize_t nread, const rbuf_t &rbuf) {
    return 0;
}

IConn *ServerGroupConn::ConnOfOrigin(const struct sockaddr *addr) {
    return IGroupConn::ConnOfOrigin(addr);
}

IConn *ServerGroupConn::ConnOfTarget(const struct sockaddr *addr) {
    return IGroupConn::ConnOfTarget(addr);
}

void ServerGroupConn::SConnObserver::OnAddrUpdated(const struct sockaddr *target, const struct sockaddr *selfAddr) {

}
