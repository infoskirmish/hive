#include <stdio.h>

#include "run_command.h"
#include "debug.h"
#include "compat.h"

#define _popen popen  
#define _pclose pclose

#define CMD_BUFF_DEFAULT_SIZE 128
#define CMD_BUFF_BYTES_TO_READ 126

int run_command(unsigned char* cmd, unsigned char* buf, int* size)
{
	unsigned char* ptr = buf;
	char temp[CMD_BUFF_DEFAULT_SIZE];
	FILE *pPipe;
	int total = 0;
	char	popen_opts[] = "r";

	if( (pPipe = _popen((char *)cmd, popen_opts)) == NULL)
	{
		perror( " popen():" );
		D(printf(" Error!\n");)
		return -1;
	}

#ifndef MIKROTIK
#if defined LINUX || defined SOLARIS
	memcpy( ptr, "\n", 1 );
	ptr += 1;
	total = 1;
#endif	// LINUX || SOLARIS
#endif	// MIKROTIK

// 128 byte buffer - 1 for the terminating NULL - 1 for the prepended \n (Linux & Solaris only) = 126
	while(fgets(temp, CMD_BUFF_BYTES_TO_READ, pPipe))
	{
		total += strlen(temp);
		if(total <= *size)
		{
			memcpy(ptr, temp, strlen(temp));
			ptr += strlen(temp);
		}
		memset(temp, 0, CMD_BUFF_DEFAULT_SIZE);
	}

	_pclose(pPipe);

	if(total > *size)
	{
		*size = total + 1;
		return 1;
	}

	return 0;
}

// testing
// can be built using:
// gcc run_command.c -DLINUX -D_TEST

#ifdef _TEST
#include <stdlib.h>
int main( void )
{
	char	*buffer = NULL;
	int		size;
	int		rv = 1;	

	// initial buffer size
	size = 2048;

	// run_command returns 1 if buffer is too small for return
	while ( rv == 1 )
	{
//		if ( buffer != NULL ) free( buffer );
//		buffer = malloc( size );
		buffer = realloc( buffer, size );
		memset( buffer, 0, size );
		rv = run_command( "ls -l", buffer, &size );
	}

	
	printf( "\n *** Return is size %d bytes *** \n\n", size );

	printf( "%s\n", buffer );

	if ( buffer != NULL ) free( buffer );

	return 0;
}
#endif
