/*
 *  farm9crypt.cpp
 *
 *  C interface between netcat and twofish.
 *
 *  Intended for direct replacement of system "read" and "write" calls.
 *
 *  Design is like a "C" version of an object.  
 *
 *  Static variables, initialized with farm9crypt_init creates a
 *  "readDecryptor" and "writeEncryptor" object, both of which are based
 *  on the assumption that text lines are being transferred between
 *  the two sides.  
 *
 *  jojo@farm9.com -- 29 Sept 2000, fixed buffer size (really it should have crashed!)
 *  jojo@farm9.com -- 2 Oct 2000, yet another bug fix...(thanks to Jimmy for reporting this!)
 *  jojo@farm9.com -- 2 Oct 2000, no more printf of key cuz its stupid (yet another Dragos suggestion)
 *  jeff@wwti.com -- 9 Feb 2001, added string.h include for yet more linux brokenness
 */

#ifndef WIN32
#include <string.h>
#include <sys/types.h>    // suggested by several people -- for OpenBSD, FreeBSD compiles
#include <sys/socket.h>		/* basics, SO_ and AF_ defs, sockaddr, ... */
#include <stdlib.h>	
#else
#include <fcntl.h>
#include <io.h>
#include <conio.h>
#include <winsock.h>
#include <time.h>
#endif

extern "C"
{
#include "farm9crypt.h"
}

#include "twofish2.h"

static int debug = false;
static int initialized = false;
static TwoFish* decryptor = NULL;
static TwoFish* encryptor = NULL;

extern "C" void farm9crypt_debug() {
	debug = true;
}

/*
 *  farm9crypt_initialized
 *
 *  Return Value:  
 *    true if this module has been initialized, otherwise false
 */

extern "C" int farm9crypt_initialized() {
	return( initialized );
}


/*
 *  farm9crypt_init
 *
 *  Input Parameters:
 *     keystr -- used to generate Twofish key for encryption and decryption
 *
 *  Return Value:
 *     none
 */

extern "C" void farm9crypt_init( char* keystr ) {
//	printf( "farm9crypt_init: %s\n", keystr );
	encryptor = new TwoFish( generateKey( keystr ), false, NULL, NULL );
	decryptor = new TwoFish( generateKey( keystr ), true, NULL, NULL );
	initialized = true;
	srand( 1000 );
}


/*
 *  farm9crypt_read
 *
 *  Susbstitute for socket read (one line replacement in netcat)
 *
 *  Handles decryption
 *
 *  Parameters same as "recv"
 */
static char outBuffer[8193];
static char inBuffer[8193];

extern "C" int farm9crypt_read( int sockfd, char* buf, int size ) {
	int total = 0;
	char outbuf[16];
	char outbuf2[16];

	if ( size > 8192 ) {
		size = 8192;
	}
	while (total < 32) {
		int result = recv( sockfd, buf + total, 32 - total, 0 );
		if ( result > 0 ) {
			total += result;
		} else {
			return(0);
		}
	}
	decryptor->resetCBC();
	decryptor->setOutputBuffer( (unsigned char*)&outBuffer[0] );
	decryptor->blockCrypt( buf, outbuf, 16 );
	decryptor->flush();
	decryptor->setOutputBuffer( (unsigned char*)&outBuffer[0] );
	decryptor->blockCrypt( buf + 16, outbuf2, 16 );
	int limit = atoi( outbuf );
	total = 0;
	char* inbuf = &inBuffer[0];

	while ( total < limit ) {
		int result = recv( sockfd, inbuf + total, limit - total, 0 );
		if ( result > 0 ) {
			total += result;
		} else {
			break;
		}
	}
	int loc = 0;
	char* obuf = &outBuffer[0];
	while ( total > 0 ) {
		int amount = 16;
		if ( total < amount ) {
			amount = total;
		}
		decryptor->blockCrypt( inbuf + loc, outbuf, amount );
		total -= amount;
		loc += 16;
	}
	decryptor->flush();
	memcpy( buf, obuf + 32, limit );
	*(buf + limit) = 0; // in case
	return( limit );
}

static char localBuf[2000];
extern "C" int farm9crypt_write( int sockfd, char* buf, int size ) {
	char tempbuf[16];
	char outbuf[16];

	sprintf( tempbuf, "%d %d", size, rand() );
	tempbuf[strlen(tempbuf)] = 'x';
	encryptor->setSocket( sockfd );
	encryptor->setOutputBuffer( (unsigned char*)&outBuffer[0] );
	encryptor->resetCBC();
	encryptor->blockCrypt( tempbuf, outbuf, 16 );
	encryptor->blockCrypt( tempbuf, outbuf, 16 );
	
	int loc = 0;
	int totalsize = size;
	while ( size > 0 ) {
		int amount = 16;
		if ( size < amount ) {
			amount = size;
		}
		encryptor->blockCrypt( buf + loc, &outBuffer[loc+32], amount );
		size -= amount;
		loc += amount;
	}
	encryptor->flush();
	return( totalsize );
}
