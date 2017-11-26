#ifdef WIN32
#include <Windows.h>
#endif

#include "bzlib.h"
#include "function_strings.h"

extern void bz_internal_error(int errcode);

//String Obfuscation
#define compress_packet wuqfkdfg4fkdz
unsigned char* compress_packet(unsigned char* packet, unsigned int packetSize, unsigned int* compressedSize);

#define release_compressed_packet quagdfosgj2lfsvbl
void release_compressed_packet(unsigned char* compressedPacket);
