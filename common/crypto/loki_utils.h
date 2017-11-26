#ifndef LOKI_UTILS_H
#define LOKI_UTILS_H

#include "crypto.h"

#ifdef WIN32
#include <Windows.h>
#endif

#ifndef WIN32
#include <inttypes.h>
#include <arpa/inet.h>
#define UINT64 uint64_t 
#define FILETIME uint64_t
#endif

#define RANDOM_LENGTH 32
//#define TOOL_ID_XOR_KEY 3
//#define TOOL_ID	0x68CAFE4A

#define DO1(buf) crc = CRCTable[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

typedef struct _Random_T {
    unsigned char random_bytes[RANDOM_LENGTH];
} Random_T, *PRandom_T;

typedef union 
{
    UINT64 ft_scalar;
    FILETIME ft_struct;
} FT;

#define HISTORY (1024)

#define VerifyBytesWithHiddenData sdthnknsddtk4mjvw

int embedData(unsigned char* data, unsigned int nToPlace, int xor_key);
int VerifyBytesWithHiddenData(unsigned char* pData, unsigned int nToVerify, int xor_key );
unsigned long CRC32(unsigned char *buf,unsigned int len, unsigned long crc);
unsigned int irand();

#endif
