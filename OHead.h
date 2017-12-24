//
// Created on 12/19/17.
//

#ifndef RSOCK_OHEAD_H
#define RSOCK_OHEAD_H


#include <array>
#include "ktype.h"
#include "rscomm.h"
#include "rstype.h"

class OHead {
public:
    // all are host endian
    OHead() = default;

    void UpdateConv(IUINT32 conv);

    IUINT32 Conv();

    void UpdateConnType(IUINT8 type);

    IUINT8 ConnType();

    void UpdateGroupId(const IdBufType &buf);

    const std::string & GroupIdStr();
    const IdBufType & GroupId();

    void UpdateDst(IUINT32 dst);

    static int GetEncBufSize();
    IUINT8 *Enc2Buf(IUINT8 *p, int len);

    static const char * DecodeBuf(OHead &head, const char *p, int len);

    static std::string BuildKey(const struct sockaddr *origin, IUINT32 conv);

    static std::string BuildAddrKey(const struct sockaddr *origin);

//    static std::string BuildGroupId(const struct sockaddr *addr);

    IUINT16 SourcePort();

    void UpdateSourcePort(IUINT16 sp);

    IUINT16 DstPort();

    void UpdateDstPort(IUINT16 dp);

    IUINT32 IncSeq();

    IUINT16 IncIpId();

    IUINT32 Dst();

//    IUINT32 Src();


public:
    // public addr, cannot be freed
    struct sockaddr *srcAddr = nullptr;

private:
    // todo: 加入len
    struct enc_head_t {
        // must correspond to fields that are gonna encoded;
        static int GetEncBufSize() {    // 15 right now
            return 0 +
//                    sizeof(HashBufType)      // hash_buf. hash buf is not computed here.
                   + sizeof(IUINT8)         // len. not used right now
                   + sizeof(IUINT8)         // resereved
                   + sizeof(IUINT8)         // send_type
                   + ID_BUF_SIZE            // id_buf
                   + sizeof(IUINT32);       // conv
        }

        // fields that are gonna encoded. {
//        HashBufType hash_buf = {0};         // detect if packet belong to rsock
        IUINT8 len = enc_head_t::GetEncBufSize();
        IUINT8 resereved = 0;
        IUINT8 conn_type = OM_PIPE_TCP;
        IdBufType id_buf = {0};
        IUINT32 conv = 0;
        // }

        bool valid;
    };

private:
    enc_head_t enc;
//    IUINT32 mConv;

    IUINT32 seq;
    IUINT16 ipid;
    IUINT8 connType;
    IUINT32 mDstAddr;
    IUINT16 mSourcePort;
    IUINT16 mDstPort;
    std::string mGroupId;
};


#endif //RSOCK_OHEAD_H
