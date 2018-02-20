//
// Created by System Administrator on 1/16/18.
//

#ifndef RSOCK_ENCHEAD_H
#define RSOCK_ENCHEAD_H

#include <cstdint>
#include "rstype.h"

struct EncHead {
public:
    enum TYPE {
        TYPE_DATA = 0,
        TYPE_CONV_RST = 1,
        TYPE_NETCONN_RST = 2,
    };

    using IntConnKeyType = uint32_t;

private:
    // fields that are gonna encoded. {
    uint8_t len = EncHead::GetMinEncSize(); // head len
    uint8_t mCmd = TYPE_DATA;
    IdBufType mIdBuf{{0}};
    uint32_t mConv = 0;
    uint32_t mConnKey = 0;
    uint8_t resereved = 0;
    // }

public:
    // must correspond to fields that are gonna encoded; 15 bytes right now
    static uint8_t GetMinEncSize();

    uint8_t GetSize();

    static const char *DecodeBuf(EncHead &, const char *p, int buf_len);

    char *Enc2Buf(char *p, int buf_len);

    EncHead &operator=(const EncHead &) = default;

    uint8_t Cmd() { return mCmd; }

    void SetCmd(uint8_t cmd) { mCmd = cmd; }

    IdBufType IdBuf() { return mIdBuf; }

    void SetIdBuf(const IdBufType &idBufType) { mIdBuf = idBufType; }

    uint32_t Conv() { return mConv; }

    void SetConv(uint32_t conv) { mConv = conv; }

    void SetConnKey(IntConnKeyType key) { mConnKey = key; }

    IntConnKeyType ConnKey() { return mConnKey; }

    static bool IsRstFlag(uint8_t cmd) { return cmd == TYPE_CONV_RST || cmd == TYPE_NETCONN_RST; }
};

#endif //RSOCK_ENCHEAD_H
