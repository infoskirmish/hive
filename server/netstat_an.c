// netan. c
//
//     Uses a lot of the features specified in busybox's 1_18_stable version
//     of the networking/netstat.c file.  We have changed some of the function
//     names and variables based upon a breif review of the code.  We will
//     use several of the function names when possible but reserve the right
//     to change accordingly since the libbb.h file will not be used.

#include "debug.h"
#include "proj_strings.h"
#include "netstat_an.h"

#if !defined _NETSTAT_AN && defined LINUX
#include <unistd.h>
#include <stdlib.h>
#include "get_data.h"

unsigned char* get_netstat_an(int* size)
{
	return get_data(size, GD_NETSTAT_AN);
}

void release_netstat_an(unsigned char* netstat_an)
{
	if(netstat_an != NULL)
	{
		free(netstat_an);
	}
}
#endif

#if defined SOLARIS
#include "get_data.h"
#include <stdlib.h>

unsigned char* get_netstat_an(int* size)
{
	return get_data(size, GD_NETSTAT_AN);
}

void release_netstat_an(unsigned char* netstat_an)
{
	if(netstat_an != NULL)
	{
		free(netstat_an);
	}
}
#endif

#if defined _NETSTAT_AN
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <pwd.h>
#include <grp.h>
#include <arpa/inet.h>

#define defaultOutputSize 16384 
static int sizeIncrementCount;
static char *outputBuffer=NULL;

//Set to 1 to read tcp6, udp6, and raw6
#define ENABLE_FEATURE_IPV6 1

//disable busybox's FAST_FUNC and other variables... 
typedef int smallint;
struct globals {
	smallint flags;
	unsigned addr_width;
};
struct globals *ptr_to_globals;
#define G (*ptr_to_globals)
#define flags            (G.flags           )

#define barrier() __asm__ __volatile__("":::"memory")

#define SET_PTR_TO_GLOBALS(x) do { \
	(*(struct globals**)&ptr_to_globals) = (void*)(x); \
	barrier(); \
} while (0)

// TODO: it appears there is memory allocated here. 
// where is it freed???
#define INIT_G() do { \
        SET_PTR_TO_GLOBALS(malloc(sizeof(G))); \
        flags = NETSTAT_CONNECTED | NETSTAT_ALLPROTO; \
} while (0)

 
#define NETSTAT_CONNECTED 0x01
#define NETSTAT_LISTENING 0x02
#define NETSTAT_NUMERIC   0x04
/* Must match getopt32 option string */
#define NETSTAT_TCP       0x10
#define NETSTAT_UDP       0x20
#define NETSTAT_RAW       0x40
#define NETSTAT_UNIX      0x80
#define NETSTAT_ALLPROTO  (NETSTAT_TCP|NETSTAT_UDP|NETSTAT_RAW|NETSTAT_UNIX)

//Note that there is a 1 to 1 mapping between the following enumeration
//  and the following tcp_state which seems to imply the following
// logic...     if (state==TCP_LISTEN) printf("%s", tcp_state[state]; 

enum {
        TCP_ESTABLISHED = 1,
        TCP_SYN_SENT,
        TCP_SYN_RECV,
        TCP_FIN_WAIT1,
        TCP_FIN_WAIT2,
        TCP_TIME_WAIT,
        TCP_CLOSE,
        TCP_CLOSE_WAIT,
        TCP_LAST_ACK,
        TCP_LISTEN,
        TCP_CLOSING, /* now a valid state */
};

//  Used to add the 6 to tcp, udp, and raw via the main routine...
static int ipv6File;

typedef enum {
        SS_FREE = 0,     /* not allocated                */
        SS_UNCONNECTED,  /* unconnected to any socket    */
        SS_CONNECTING,   /* in process of connecting     */
        SS_CONNECTED,    /* connected to socket          */
        SS_DISCONNECTING /* in process of disconnecting  */
} socket_state;

#define SO_ACCEPTCON (1<<16)  /* performed a listen           */
#define SO_WAITDATA  (1<<17)  /* wait data to read            */
#define SO_NOSPACE   (1<<18)  /* no space to write            */

#define ADDR_NORMAL_WIDTH        23
/* When there are IPv6 connections the IPv6 addresses will be
 * truncated to none-recognition. The '-W' option makes the
 * address columns wide enough to accomodate for longest possible
 * IPv6 addresses, i.e. addresses of the form
 * xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:ddd.ddd.ddd.ddd
 */
#define ADDR_WIDE                51  /* INET6_ADDRSTRLEN + 5 for the port number */

# define FMT_NET_CONN_DATA_A       "%5s   %6ld %6ld %-25s %-25s %-12s"
# define NEW_FMT_NET_CONN_DATA_A       "%5s   %6ld %6ld %-25s %-25s %-12s \n"
# define FMT_NET_CONN_HEADER_A     "\nProto   Recv-Q Send-Q %-25s %-25s State       %s\n"
# define NEW_FMT_NET_CONN_HEADER_A     "\nProto   Recv-Q Send-Q %-25s %-25s State\n"

static void build_ipv6_addr(char* local_addr, struct sockaddr_in6* localaddr)
{
        char addr6[INET6_ADDRSTRLEN];
        struct in6_addr in6;

        sscanf(local_addr, "%08X%08X%08X%08X",
                   &in6.s6_addr32[0], &in6.s6_addr32[1],
                   &in6.s6_addr32[2], &in6.s6_addr32[3]);
        inet_ntop(AF_INET6, &in6, addr6, sizeof(addr6));
        inet_pton(AF_INET6, addr6, &localaddr->sin6_addr);

        localaddr->sin6_family = AF_INET6;
}

static void build_ipv4_addr(char* local_addr, struct sockaddr_in* localaddr)
{
        sscanf(local_addr, "%X", &localaddr->sin_addr.s_addr);
        localaddr->sin_family = AF_INET;
}

static char *get_sname(int port, const char *proto, int numeric )
{
	char* lastChancePort;
	//D(printf("\n\n Within get_sname, port = %i, proto=%s, numeric=%i\n\n", port, proto, numeric);)
	//D(printf("\n\n port = %X\n", port);)
	//D(printf(" ntohs(port) = %X\n", ntohs(port));)
        
	// this memory is free'd by the caller, ip_port_str()
	lastChancePort = (char *) malloc(20);

	if (!port)
	{
//		return 0;
		strcpy( lastChancePort, "*" );
		return lastChancePort;
	}

	if (!numeric) {
		struct servent *se = getservbyport(port, proto);

		if (se)
		{
			strcpy( lastChancePort, se->s_name );
			return lastChancePort;
		}
	}

	memset( lastChancePort, '\0', 20);
	snprintf(lastChancePort, 19, "%d", ntohs(port));
	return lastChancePort;       //THIS IS NOT PART OF THE CODE BUT GETS AROUND itoa

}

// Die with an error message if we can't malloc() enough space and do an
// sprintf() into that space.
char*  xasprintf(const char *format, ...)
{
        va_list p;
        int r;
        char *string_ptr;

        va_start(p, format);
        r = vasprintf(&string_ptr, format, p);
        va_end(p);

        if (r < 0)
				exit(1);
                //bb_error_msg_and_die(bb_msg_memory_exhausted);
        return string_ptr;
}


static char *ip_port_str(struct sockaddr *addr, int port, const char *proto, int numeric)
{
        char *host, *host_port, *sname;

        /* Code which used "*" for INADDR_ANY is removed: it's ambiguous
         * in IPv6, while "0.0.0.0" is not. */

	//D(printf("\n port = %i\n", port);)

	// host is free'd at the end of this function
	// TODO: why malloc(25) if true or false
	// why not just malloc(25) & avoid the terinary conditional
        host = numeric ? malloc(25)
                       : malloc(25);
	
	switch (addr->sa_family)
	{
		case AF_INET:
			//inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), host, sizeof(addr)-1);
			inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), host, INET_ADDRSTRLEN);
			//D(printf("\nip address = %s\n", host);)
			break;
		case AF_INET6:
			//inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), host, sizeof(addr)-1);
			inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)addr)->sin6_addr), host, INET6_ADDRSTRLEN);
			//D(printf("\nip address = %s\n", host);)
			break;
		default:
			//D(printf("Non AF_INET address.\n");)
			break;
	}
	//D(printf("\nhostIp = %s\n", host);)

	// xasprintf mallocs memory which is free'd by new_print_inet_line() which calls ip_port_str()
        sname = get_sname(htons(port), proto, numeric);
        host_port = xasprintf( "%s:%s", host, sname );

		if ( sname != NULL ) free( sname );
        if (host) free(host);
        return host_port;
}


struct inet_params {
        int local_port, rem_port, state, uid;
        union {
                struct sockaddr     sa;
                struct sockaddr_in  sin;
                struct sockaddr_in6 sin6;
        } localaddr, remaddr;
        unsigned long rxq, txq, inode;
};

static int scan_inet_proc_line(struct inet_params *param, char *line)
{
        int num;
        /* IPv6 /proc files use 32-char hex representation
         * of IPv6 address, followed by :PORT_IN_HEX
         */
        char local_addr[33], rem_addr[33]; /* 32 + 1 for NUL */

        num = sscanf(line,
                        "%*d: %32[0-9A-Fa-f]:%X "
                        "%32[0-9A-Fa-f]:%X %X "
                        "%lX:%lX %*X:%*X "
                        "%*X %d %*d %ld ",
                        local_addr, &param->local_port,
                        rem_addr, &param->rem_port, &param->state,
                        &param->txq, &param->rxq,
                        &param->uid, &param->inode);
        if (num < 9) 
	{
                return 1; /* error */
        }

        if (strlen(local_addr) > 8) {
                build_ipv6_addr(local_addr, &param->localaddr.sin6);
                build_ipv6_addr(rem_addr, &param->remaddr.sin6);
        } else {
                build_ipv4_addr(local_addr, &param->localaddr.sin);
                build_ipv4_addr(rem_addr, &param->remaddr.sin);
        }

	//D(printf("scan_inet returned 0.\n");)

        return 0;
}

static void new_print_inet_line(struct inet_params *param,
                int state , const char *proto, int is_connected)
{

	char *oldOutput=NULL;
	char tempString[1024];
	is_connected = is_connected;    //Get's rid of compiler complaint...
	char state_str[256]; 

   char *l = ip_port_str( &param->localaddr.sa, param->local_port,
                          proto, flags & NETSTAT_NUMERIC);
	//D(printf("\nl = %s\n", l);)

   char *r = ip_port_str( &param->remaddr.sa, param->rem_port,
                          proto, flags & NETSTAT_NUMERIC);
 	//D(printf("\nr = %s\n", r);)

    memset( state_str, '\0', sizeof(state_str) );
	switch (state)
	{
		case 0:
			snprintf(state_str, sizeof(state_str)-1, " ");  //""
			break;
		case TCP_ESTABLISHED:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan14);  //"ESTABLISHED"
			break;
		case TCP_SYN_SENT: 
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan21);  //"SYN_SENT"
			break;
		case TCP_SYN_RECV:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan22);  //"SYN_RECV"
			break;
		case TCP_FIN_WAIT1:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan23);  //"FIN_WAIT1"
			break;
		case TCP_FIN_WAIT2:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan24);  //"FIN_WAIT2"
			break;
		case TCP_TIME_WAIT:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan25);  //"TIME_WAIT"
			break;
		case TCP_CLOSE:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan26);  //"CLOSE"
			break;
		case TCP_CLOSE_WAIT:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan27);  //"CLOSE_WAIT"
			break;
		case TCP_LAST_ACK:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan28);  //"LAST_ACK"
			break;
		case TCP_LISTEN:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan29);  //"LISTEN"
			break;
		case TCP_CLOSING:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan30);  //"CLOSING"
			break;
		default:
			snprintf(state_str, sizeof(state_str)-1, "%s", ntsan31);  //"UNKNOWN"
			break;
	}


	//Store the information into the outputBuf now instead of printing out to standard output
	//printf(FMT_NET_CONN_DATA_A, proto, param->rxq, param->txq, l, r, state_str);
	// TODO: for good measure, memset( tempString, 0, 1000 )
	// TODO: snprintf( dest, dest_size - 1, format, ... )
	//snprintf( tempString, 1000, NEW_FMT_NET_CONN_DATA_A, proto, param->rxq, param->txq, l, r, state_str);  //SPILL
	memset( tempString, '\0', sizeof(tempString) ); 
	snprintf( tempString, sizeof(tempString)-1, NEW_FMT_NET_CONN_DATA_A, proto, param->rxq, param->txq, l, r, state_str); 
	//Make sure concatenation fits...
	if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(outputBuffer) )
	{
			if (outputBuffer) oldOutput = outputBuffer;
			sizeIncrementCount++;
			outputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
			memset( outputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
			if (oldOutput) 
			{
				strncat( outputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(outputBuffer) - 1);
				free(oldOutput);
				oldOutput = NULL;
			}
	}
	strncat( outputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(outputBuffer) - 1);

   if (l) free(l);
   if (r) free(r);

}

static int  new_tcp_do_one(char *line)
{
        struct inet_params param;
		  char protoType[256];

        memset(&param, 0, sizeof(param));

        if (scan_inet_proc_line(&param, line))
                return 1;

	if( ipv6File==0)
	{
			memset( protoType, '\0', sizeof(protoType) );
	        snprintf( protoType, sizeof(protoType) - 1, "%s", ntsan17);   //tcp
	}
	else
	{
			memset( protoType, '\0', sizeof(protoType) );
	        snprintf( protoType, sizeof(protoType) - 1, "%s", ntsan16);   //tcp6
	}
	     //new_print_inet_line(&param, tcp_state[param.state], protoType, param.rem_port);
	     new_print_inet_line(&param, param.state, protoType, param.rem_port);
        return 0;
}


# define NOT_NULL_ADDR(A) ( \
        ( (A.sa.sa_family == AF_INET6) \
          && (A.sin6.sin6_addr.s6_addr32[0] | A.sin6.sin6_addr.s6_addr32[1] | \
              A.sin6.sin6_addr.s6_addr32[2] | A.sin6.sin6_addr.s6_addr32[3])  \
        ) || ( \
          (A.sa.sa_family == AF_INET) \
          && A.sin.sin_addr.s_addr != 0 \
        ) \
)


static int  new_udp_do_one(char *line)
{
	int have_remaddr;
	struct inet_params param;
	char protoType[256];
	int state;

	memset(&param, 0, sizeof(param)); /* otherwise we display garbage IPv6 scope_ids */
	if (scan_inet_proc_line(&param, line))
		return 1;

	//Why doesn't it pass in the param.state value?
	state = 31; //UNKNOWN
	switch (param.state) {
			case TCP_ESTABLISHED:
				state = 1;
			case TCP_CLOSE:
				state = 0;
				break;
	}

	have_remaddr = NOT_NULL_ADDR(param.remaddr);

	memset(protoType, '\0', sizeof(protoType) );
	if( ipv6File==0)
	{
        snprintf(protoType, sizeof(protoType)-1, "%s", ntsan12); //udp
	}
	else
	{
        snprintf(protoType, sizeof(protoType)-1, "%s", ntsan13); //udp6
	}
   	new_print_inet_line(&param, state, protoType, have_remaddr);

	return 0;
}


static int  new_raw_do_one(char *line)
{
	int have_remaddr;
	struct inet_params param;
	char protoType[256];

	if (scan_inet_proc_line(&param, line))
		return 1;

	memset(protoType, '\0', sizeof(protoType) );
	have_remaddr = NOT_NULL_ADDR(param.remaddr);
	//print_inet_line(&param, itoa(param.state), "raw", have_remaddr);
	if( ipv6File==0)
	{
       	snprintf(protoType, sizeof(protoType)-1, "%s", ntsan10); //raw
	}
	else
	{
        snprintf(protoType, sizeof(protoType)-1, "%s", ntsan11); //raw6
	}
	new_print_inet_line(&param, param.state, protoType, have_remaddr);

	return 0;
}


static char *xmalloc_fgets_internal(FILE *file, const char *terminating_string, int chop_off, size_t *maxsz_p) 
{
	char *linebuf = NULL;
	const int term_length = strlen(terminating_string);
	int end_string_offset;
	unsigned int linebufsz = 0;
	unsigned int idx = 0;
	int ch;
	//size_t maxsz = *maxsz_p;    //MRR
	unsigned int maxsz = (unsigned int) *maxsz_p;

	while (1) {
		ch = fgetc(file);
		if (ch == EOF) {
			if (idx == 0)
				return linebuf; /* NULL */
			break;
		}

		if (idx >= linebufsz) {
			linebufsz += 200;
			linebuf = (char *) realloc(linebuf, linebufsz);
			if (idx >= maxsz) {
				linebuf[idx] = ch;
				idx++;
				break;
			}
		}

		linebuf[idx] = ch;
		idx++;

		/* Check for terminating string */
		end_string_offset = idx - term_length;
		if (end_string_offset >= 0
		 && memcmp(&linebuf[end_string_offset], terminating_string, term_length) == 0)
		{
			if (chop_off)
				idx -= term_length;
			break;
		}
	}
	/* Grow/shrink *first*, then store NUL */
	linebuf = (char *) realloc(linebuf, idx + 1);
	linebuf[idx] = '\0';
	*maxsz_p = idx;
	return linebuf;
}

/* Read up to TERMINATING_STRING from FILE and return it,
 * including terminating string.
 * Non-terminated string can be returned if EOF is reached.
 * Return NULL if EOF is reached immediately.  */
char*  xmalloc_fgets_str(FILE *file, const char *terminating_string)
{
	size_t maxsz = INT_MAX - 4095;
	return xmalloc_fgets_internal(file, terminating_string, 0, &maxsz);
}

static void new_do_info(const char *file, int  (*proc)(char *))
{
        int lnr;
        FILE *procinfo;
        char *buffer;

        /* _stdin is just to save "r" param */
        //procinfo = fopen_or_warn_stdin(file);
        procinfo = fopen( file, "r");       //Bypass of busybox file open...
        if (procinfo == NULL) {
                return;
        }

	//D(printf("\n Opened file %s.\n", file);)

        lnr = 0;
        /* Why xmalloc_fgets_str? because it doesn't stop on NULs */
        while ((buffer = xmalloc_fgets_str(procinfo, "\n")) != NULL) {
                /* line 0 is skipped */
                if (lnr && proc(buffer))
					 {
                        //D(printf("%s: bogus data on line %d", file, lnr + 1);) //Bypass busybox bb_error_msg on the next line...
                        //bb_error_msg("%s: bogus data on line %d", file, lnr + 1);
                }
					 else
					 {
					 		//D(printf("\n\n buffer = %s\n\n", buffer);)
					 		//outputBuf = doInfoBuffer;
					 }
					 lnr++;
					 //D(printf(" line number = %d.\n", lnr);)
                if (buffer) free(buffer);
        }
        fclose(procinfo);

		
	//D(printf("       Closed file %s.\n", file);)
}

//Called by beacon.c to release data 
void release_netstat_an(unsigned char* netstat_an)
{
	if(netstat_an != NULL)
	{
		free(netstat_an);
		netstat_an = NULL;
	}
	return;
}

//int get_netstat_an(unsigned char* buf, int* size)
unsigned char* get_netstat_an( int* size)
{

	char *oldOutput=NULL;
	int outputLength = 0;
	char tempString[1024];
	char fileName[256];
	*size = 0;

	outputBuffer = NULL;

	INIT_G();  //Initializes global flags...
	sizeIncrementCount=1;
		
	outputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
	memset( outputBuffer, '\0', sizeIncrementCount*defaultOutputSize );

	//Print Header but send it to buf now...
	//printf("\nActive Internet connections (servers and established)");
	//printf(FMT_NET_CONN_HEADER_A, "Local Address", "Foreign Address", "");
	//printf(" First writing to tempString.\n");
	//snprintf( tempString, 1000, "\nActive Internet connections (servers and established)");
	memset( tempString, '\0', sizeof(tempString) );
	snprintf( tempString, sizeof(tempString), "%s", ntsan7);
	//Make sure concatenation fits...
	if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(outputBuffer) )
	{
			if (outputBuffer) oldOutput = outputBuffer;
			sizeIncrementCount++;
			outputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
			memset( outputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
			if (oldOutput)
			{
				strncat( outputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(outputBuffer) - 1);
				free(oldOutput);
				oldOutput = NULL;
			}
	}
	strncat( outputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(outputBuffer) - 1);

	//snprintf( tempString, 1000, NEW_FMT_NET_CONN_HEADER_A, "Local Address", "Foreign Address");
	memset( tempString, '\0', sizeof(tempString) );
	snprintf( tempString, sizeof(tempString), NEW_FMT_NET_CONN_HEADER_A, ntsan8, ntsan9);
	//Make sure concatenation fits...
	if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(outputBuffer) )
	{
			if (outputBuffer) oldOutput = outputBuffer;
			sizeIncrementCount++;
			outputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
			memset( outputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
			if (oldOutput)
			{
				strncat( outputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(outputBuffer) - 1);
				free(oldOutput);
				oldOutput = NULL;
			}
	}
	strncat( outputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(outputBuffer) - 1);

//tcp socket information
	ipv6File=0;
	//new_do_info("/proc/net/tcp", new_tcp_do_one);
	memset( fileName, '\0', sizeof(fileName) );
	snprintf(fileName, sizeof(fileName)-1, "%s", ntsan1);
	new_do_info(fileName, new_tcp_do_one);

	ipv6File=1;
	//new_do_info("/proc/net/tcp6", new_tcp_do_one);
	memset( fileName, '\0', sizeof(fileName) );
	snprintf(fileName, sizeof(fileName)-1, "%s", ntsan2);
	new_do_info(fileName, new_tcp_do_one);

//udp socket information
	ipv6File=0;
	//new_do_info("/proc/net/udp", new_udp_do_one);
	memset( fileName, '\0', sizeof(fileName) );
	snprintf(fileName, sizeof(fileName)-1, "%s", ntsan3);
	new_do_info(fileName, new_udp_do_one);
	ipv6File=1;
	//new_do_info("/proc/net/udp6", new_udp_do_one);
	memset( fileName, '\0', sizeof(fileName) );
	snprintf(fileName, sizeof(fileName)-1, "%s", ntsan4);
	new_do_info(fileName, new_udp_do_one);

//raw socket information													        
	ipv6File=0;
	//new_do_info("/proc/net/raw", new_raw_do_one);
	memset( fileName, '\0', sizeof(fileName) );
	snprintf(fileName, sizeof(fileName)-1, "%s", ntsan5);
	new_do_info(fileName , new_raw_do_one);
	ipv6File=1;
	//new_do_info("/proc/net/raw6", new_raw_do_one);
	memset( fileName, '\0', sizeof(fileName) );
	snprintf(fileName, sizeof(fileName)-1, "%s", ntsan6);
	new_do_info(fileName, new_raw_do_one);

	// Do not include unix sockets thus far...
	//printf("\nProto RefCnt Flags       Type       State         I-Node %sPath\n", "Program ");
	//do_info("/proc/net/unix", unix_do_one);

// TODO: remove the business re: finalOutput
	outputLength= strlen(outputBuffer) + 1;

	if (oldOutput) free(oldOutput);
	oldOutput = NULL;
	
	
	// j: add to try to free what's allocated from INIT_G
	if ( ptr_to_globals != NULL ) free( ptr_to_globals );

	if (outputLength > 1)
	{

		*size = sizeIncrementCount * defaultOutputSize;
		return (unsigned char *) outputBuffer;

	}
	else
	{
		*size = 0;
		if (outputBuffer) free(outputBuffer);
		return NULL;
	}

}
#endif
