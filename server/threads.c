#include "compat.h"
#include "debug.h"
#include "threads.h"
#include "daemonize.h"

//******************************************************************
#if defined LINUX || defined SOLARIS
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

//******************************************************************
//************** Platform specific functions ***********************
//******************************************************************
#if defined LINUX || defined SOLARIS

int make_thread( void *(*func)(void *), void *args )
{
	pthread_t	thread_id;

	if ( pthread_create( &thread_id, NULL, func, (void *)args ) != SUCCESS )
	{
		perror( " pthread_create()" );
		return FAILURE;
	}

	return SUCCESS;
}

int terminate_thread( void )
{
	return SUCCESS;
}

int fork_process( void *(*func)(void *), void *args )
{
	pid_t	pid;

	pid = fork();

	if ( pid < 0 )			// error
	{
		return pid;
	}
	else if ( pid != 0 )	// parent
	{
		// wait for the child process to exit.  this will happen relatively quickly b/c the child will daemonize
		waitpid( pid, NULL, 0 );
		return SUCCESS;
	}
	
	// else, we are the child
#ifndef DEBUG
	daemonize();
#endif
	func( args );

	return pid;	
}

#endif
//******************************************************************
