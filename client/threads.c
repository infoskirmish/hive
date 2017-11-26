#include <pthread.h>

#include "debug.h"
#include "threads.h"
#include "hclient.h"


//******************************************************************
int make_thread( void *(*func)(void *), void *args )
{
	int				err;
	pthread_t		tid;
	pthread_attr_t	attr;
	
	err = pthread_attr_init( &attr );

	if ( err != 0 ) return( err );

	err = pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

	if ( err == 0 )
	{
		err = pthread_create( &tid, &attr, func, args );

		if ( err != SUCCESS )
		{
			perror( " pthread_create()" );
			return FAILURE;
		}
	}

//	err = pthread_create( &tid, NULL, func, args );
//	printf( " DEBUG: pthread_create() returned %d\n", err );

	pthread_attr_destroy( &attr );

	return err;
}

//******************************************************************
int terminate_thread( void )
{
	return SUCCESS;
}

//******************************************************************
