//
// Created on 12/17/17.
//

#include "SRawConn.h"
#include "SConn.h"

void SRawConn::capInput(struct omhead_t *head, struct sockaddr_in *addr, char *data, int len) {
    auto group = conn->ConnOfOrigin(reinterpret_cast<const sockaddr *>(addr));
    if (!group) {
        IGroupConn *newGroup = new ServerGroupConn(head->conn_id, nullptr);
        group = newGroup;
        IConn *newConn = new SConn(0, nullptr, nullptr);
        newGroup->AddConn(newConn, reinterpret_cast<const sockaddr *>(addr));
    }
    rbuf_t rbuf;
    rbuf.base = data;
    rbuf.len = len;
    rbuf.data = head;
    group->Input(len, rbuf);
}
