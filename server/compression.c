#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "compression.h"
#include "debug.h"

void bz_internal_error(int errcode)
{
	(void)errcode;
	return;
}

unsigned char* compress_packet(unsigned char* packet, unsigned int packetSize, unsigned int* compressedSize)
{
	unsigned char* temp = NULL;
	int ret = 0;

	*compressedSize = packetSize;

	temp = (unsigned char*) malloc(*compressedSize);
	memset(temp,0,*compressedSize);

	ret = BZ2_bzBuffToBuffCompress((char*)temp,compressedSize,(char*)packet,packetSize,9,0,30);
	if(ret != BZ_OK)
	{
		DLX(4, printf("Compression failed with error: %0x", ret));
		return NULL;
	}

	return temp;
}

void release_compressed_packet(unsigned char* compressedPacket)
{
	if(compressedPacket != NULL)
	{
		free(compressedPacket);
	}
}
