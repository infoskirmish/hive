/*
 *  farm9crypt.h
 *
 *  C interface between netcat and twofish.
 *
 *  Intended for direct replacement of system "read" and "write" calls.
 *
 *  NOTE: This file must be included within "extern C {...}" when included in C++
 */
void farm9crypt_init( char* inkey );
void farm9crypt_debug();
int farm9crypt_initialized();
int farm9crypt_read( int sockfd, char* buf, int size );
int farm9crypt_write( int sockfd, char* buf, int size );

