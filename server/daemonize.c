#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined LINUX || defined SOLARIS || defined UCLIBC
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "daemonize.h"

#if defined UCLIBC || defined SOLARIS
int daemonize( void )
{
	int	i;
	pid_t	pid;
	char	devnull[10];

#ifdef SOLARIS
	// set process max core file size to zero.
	struct rlimit 	corelimit = { 0, 0 };
#endif

	devnull[0] = '/'; devnull[5] = 'n';
    devnull[1] = 'd'; devnull[6] = 'u';
    devnull[2] = 'e'; devnull[7] = 'l';
    devnull[3] = 'v'; devnull[8] = 'l';
    devnull[4] = '/'; devnull[9] = '\0';

	pid = fork();

	if ( pid < 0 )
	{
		return( -1 );
	}

	else if ( pid != 0 )
	{
		exit( 0 );
	}

	/* CHILD CONTINUES */

	/* become session leader */
	if ( setsid() < 0 )
	{
		return (-1);
	}

	signal( SIGHUP, SIG_IGN );
	
	pid = fork();
	
	if ( pid < 0 )
	{
		return( -1 );
	}

	else if ( pid != 0 )		// child 1 terminates
	{
		exit( 0 );
	}

	/* CHILD 2 CONTINUES */
	chdir( "/" ); 
	
	/* close file descriptors */
	for ( i = 0; i <= 2; i++ )
	{
		close( i );
	}

	open( devnull, O_RDONLY );
	open( devnull, O_RDWR );
	open( devnull, O_RDWR );

#ifdef SOLARIS
	// set Solaris max core file size to zero
	setrlimit( RLIMIT_CORE, &corelimit );
#endif

	return( 0 );
}

#elif defined LINUX
int daemonize( void )
{
	return daemon( 0, 0 );
}
 
#endif
