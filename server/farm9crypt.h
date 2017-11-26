/*
 *  farm9crypt.h
 *
 *  C interface between netcat and twofish.
 *
 *  Intended for direct replacement of system "read" and "write" calls.
 *
 *  NOTE: This file must be included within "extern C {...}" when included in C++
 */

#include "port.h"

//String Obfuscation
#define farm9crypt_init eieqroadjs
void farm9crypt_init( char* inkey );

#define farm9crypt_debug oiwprwzfgw1d
void farm9crypt_debug( void );

#define farm9crypt_initialized uersfisvkf3q
int farm9crypt_initialized( void );

#define farm9crypt_read kilsdfg4fsf
int farm9crypt_read( int sockfd, char* buf, int size );

#define farm9crypt_write eiqdfwegjddf56d
int farm9crypt_write( int sockfd, char* buf, int size );

