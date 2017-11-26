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
#include <string.h>
#include <sys/types.h>    // suggested by several people -- for OpenBSD, FreeBSD compiles
#include <sys/socket.h>		/* basics, SO_ and AF_ defs, sockaddr, ... */
#include <stdlib.h>	
#include <unistd.h>

#include "farm9crypt.h"
#include "twofish.h"
#include "port.h"
#include "debug.h"

static int initialized = false;

//String Obfuscation
#define encryptor hsdfkwrthadsrg7wq
#define decryptor jadubtbacd2dhf
struct tf_context encryptor, decryptor;

/*
 *  farm9crypt_initialized
 *
 *  Return Value:  
 *    true if this module has been initialized, otherwise false
 */

int farm9crypt_initialized( void ) {
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

void farm9crypt_init( char* keystr ) {

	DLX(4, printf("key string: %s\n", keystr));

	tf_init( &encryptor, generateKey( keystr ), false, NULL, NULL );

	tf_init( &decryptor, generateKey( keystr ), true, NULL, NULL );

	initialized = true;

//	srand( 1000 );

	return;
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

int farm9crypt_read( int sockfd, char* buf, int size ) {

	int total = 0;
	char outbuf[16];
	char outbuf2[16];

	if ( initialized != true )
	{
		DLX(4, printf("cipher not initialized\n"));
		return -1;
	}

#ifdef BYPASS
	int	rv = 0;
	rv = read( sockfd, buf, size );
	return rv;
#endif

	if ( size > 8192 ) {
		size = 8192;
	}

	// read from 
	while (total < 32) {
		int result = recv( sockfd, buf + total, 32 - total, 0 );
		if ( result > 0 ) {
			total += result;
			//D( printf( " . DEBUG: %i bytes read, %i bytes total. %i\n", result, total, __LINE__ ); )
		} else {
			//D( printf( " . DEBUG: %i bytes read, %i bytes total. %i\n", result, total, __LINE__ ); )
			return(0);
		}
	}

	tf_resetCBC( &decryptor );
	tf_setOutputBuffer( &decryptor, (unsigned char*)&outBuffer[0] );
	tf_blockCrypt( &decryptor, buf, outbuf, 16 );
	tf_flush( &decryptor );
	tf_setOutputBuffer( &decryptor, (unsigned char*)&outBuffer[0] );
	tf_blockCrypt( &decryptor, buf + 16, outbuf2, 16 );

	int limit = atoi( outbuf );
	total = 0;
	char* inbuf = &inBuffer[0];
	
	DLX(4, printf("expecting %i bytes\n", limit));

	while ( total < limit ) {
		int result = recv( sockfd, inbuf + total, limit - total, 0 );
		if ( result > 0 ) {
			total += result;
			//D( printf( " . DEBUG: %i bytes read, %i bytes total of %i bytes. %i\n", result, total, limit,  __LINE__ ); )
		} else {
			//D( printf( " . DEBUG: %i bytes read, %i bytes total of %i bytes. %i\n", result, total, limit,  __LINE__ ); )
			break;
		}
	}

	int loc = 0;
	char* obuf = &outBuffer[0];

//	D( printf( " . DEBUG: line %i\n", __LINE__ ); )
	while ( total > 0 ) {
		int amount = 16;
		if ( total < amount ) {
			amount = total;
		}
		tf_blockCrypt( &decryptor, inbuf + loc, outbuf, amount );
		total -= amount;
		loc += 16;
	}
	tf_flush( &decryptor );
	memcpy( buf, obuf + 32, limit );
	*(buf + limit) = 0; // in case
	return( limit );
}

//static char localBuf[2000];
int farm9crypt_write( int sockfd, char* buf, int size ) {

	char tempbuf[16];
	char outbuf[16];

	if ( initialized != true )
	{
		DLX(4, printf("cipher not initialized\n"));
		return -1;
	}

#ifdef BYPASS
	int 	rv = 0;
	rv = write( sockfd, buf, size );
	return rv;
#endif

	memset( tempbuf, 0, 16 );
	memset( outbuf, 0, 16 );
	memset( outBuffer, 0, 8193 );

	sprintf( tempbuf, "%d %d", size, rand() );
	tempbuf[15] = '\0';
//	tempbuf[strlen(tempbuf)] = 'x';

	tf_setSocket( &encryptor, sockfd );
	tf_setOutputBuffer( &encryptor, (unsigned char*)&outBuffer[0] );
	tf_resetCBC( &encryptor );
	tf_blockCrypt( &encryptor, tempbuf, outbuf, 16 );
	tf_blockCrypt( &encryptor, tempbuf, outbuf, 16 );
	
	int loc = 0;
	int totalsize = size;
	//D( printf( " . DEBUG: line %i\n", __LINE__ ); )
	while ( size > 0 ) {
		int amount = 16;
		if ( size < amount ) {
			amount = size;
		}
		tf_blockCrypt( &encryptor, buf + loc, &outBuffer[loc+32], amount );
		size -= amount;
		loc += amount;
	}
	tf_flush( &encryptor );
	return( totalsize );
}
