//
// Created on 10/30/17.
//

#ifndef SOCKNM_ENC_H
#define SOCKNM_ENC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

char *encode_uint32(uint32_t i, char *p);

char *encode_uint16(uint16_t i, char *p);

char *encode_uint8(uint8_t i, char *p);

const char *decode_uint32(uint32_t *i, const char *p);

const char *decode_uint16(uint16_t *i, const char *p);

const char *decode_uint8(uint8_t *i, const char *p);

int8_t is_little_endian();

void big_endian_to_little(uint32_t i, char *p);

#ifdef __cplusplus
}
#endif
#endif //SOCKNM_ENC_H
