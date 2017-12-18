//
// Created on 10/30/17.
//

#ifndef SOCKNM_ENC_H
#define SOCKNM_ENC_H

#include "ktype.h"

#ifdef __cplusplus
extern "C" {
#endif

char *encode_uint32(IUINT32 i, char *p);

char *encode_uint16(IUINT16 i, char *p);

char *encode_uint8(IUINT8 i, char *p);

const char *decode_uint32(IUINT32 *i, const char *p);

const char *decode_uint16(IUINT16 *i, const char *p);

const char *decode_uint8(IUINT8 *i, const char *p);

IINT8 is_little_endian();

void big_endian_to_little(IUINT32 i, char *p);

#ifdef __cplusplus
}
#endif
#endif //SOCKNM_ENC_H
