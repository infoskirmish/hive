#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "shuffle.h"
#include "farm9crypt.h"
#include "debug.h"

#define max(x,y) ((x) > (y) ? (x) : (y))

int shuffle( int localfd, int netfd )
{
	fd_set 			rfds;
	struct timeval 	tv;
	int 			rv;
	char 			buffer[8196];

	if( netfd > FD_SETSIZE )
	{
		DLX(4, printf("Invalid file descriptor %d\n", netfd));
		return ( -1 );
	}

	if( localfd > FD_SETSIZE )
	{
		DLX(4, printf("Invalid file descriptor %d\n", localfd));
		return ( -1 );
	}

	for( ;; )
	{

		FD_ZERO( &rfds );
		FD_SET( localfd, &rfds );
		FD_SET( netfd, &rfds );

		tv.tv_sec = 5;
		tv.tv_usec = 0;

		rv = select( max( localfd, netfd ) + 1, &rfds, NULL, NULL, &tv );

		if( rv == -1 && errno == EINTR )
			continue;

		if( rv < 0 )
		{
			D( perror( " ! select()" ); )
			return -1;
		}

		if( FD_ISSET( netfd, &rfds ) )
		{
			// network is ready. read from network and write to stdout
			memset( buffer, 0, 8196 );

			// read from network
			rv = farm9crypt_read( netfd, buffer, 8196 );
			if( rv == 0 )
			{
				DLX(4, printf("socket closed.\n"));
				return 0;
			}
			else if( rv < 0 )
			{
				DLX(4, perror("read() from socket"));
				return -1;
			}

			// write to localfd
			rv = write( localfd, buffer, rv );
			if( rv == 0 )
			{
				DLX(4, printf("stdout closed.\n"));
				return 0;
			}
			else if( rv < 0 )
			{
				DLX(4, perror("write() to localfd"));
				return -1;
			}
		}

		if( FD_ISSET( localfd, &rfds ) )
		{
			// stdin is ready. read stdin and write to network
			memset( buffer, 0, 8196 );

			// read from localfd
			rv = read( localfd, buffer, 8196 );
			if( rv == 0 )
			{
				DLX(4, printf("localfd closed\n"));
				return 0;
			}
			else if( rv < 0 )
			{
				DLX(4, perror("read() from socket\n"));
				return -1;
			}

			// write to stdout
			rv = farm9crypt_write( netfd, buffer, rv );
			if( rv == 0 )
			{
				DLX(4, printf("socket closed.\n"));
				return 0;
			}
			else if( rv < 0 )
			{
				DLX(4, printf("write() to socket"));
				return -1;
			}
		}
	}

}	/* shuffle() */
