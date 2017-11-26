/* include get_interface */
#include "ifconfig.h"
#include "proj_strings.h"
#include "debug.h"

// usually, non-MikroTik, Linux hosts
#if !defined _IPCONFIG && defined LINUX
#include <unistd.h>
#include <stdlib.h>
#include "get_data.h"
unsigned char* get_ifconfig(int* size)
{
	return get_data(size,GD_IPCONFIG);
}

void release_ifconfig(unsigned char* ifconfig)
{
	if (ifconfig != NULL)
	{
		free(ifconfig);
	}

}
#endif

#if defined SOLARIS
#include "get_data.h"
#include <stdlib.h>

unsigned char* get_ifconfig(int* size)
{
	return get_data(size,GD_IPCONFIG);
}

void release_ifconfig(unsigned char* ifconfig)
{
	if (ifconfig != NULL)
	{
		free(ifconfig);
	}

}
#endif

// internal routines from here until EOF.  usually used on the MikroTik builds
// that don't have ifconfig shell commands
#if defined _IPCONFIG

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "error.h"
#include <errno.h>

#define _PATH_PROCNET_DEV		"/proc/net/dev"
#define _PATH_PROCNET_IFINET6	"/proc/net/if_inet6"

#ifdef	DEBUG
#define D(x)	x
#else
#define D(x)
#endif

#define IPV6 1

#define IPV6_ADDR_ANY           0x0000U

#define IPV6_ADDR_UNICAST       0x0001U
#define IPV6_ADDR_MULTICAST     0x0002U
#define IPV6_ADDR_ANYCAST       0x0004U

#define IPV6_ADDR_LOOPBACK      0x0010U
#define IPV6_ADDR_LINKLOCAL     0x0020U
#define IPV6_ADDR_SITELOCAL     0x0040U

#define IPV6_ADDR_COMPATv4      0x0080U

#define IPV6_ADDR_SCOPE_MASK    0x00f0U

#define IPV6_ADDR_MAPPED        0x1000U
#define IPV6_ADDR_RESERVED      0x2000U /* reserved address space */


//Global code that ifconfig results will be written to...
#define defaultOutputSize 16384 
static char *iOutputBuffer;
static int sizeIncrementCount;

//Called by beacon.c to release data in beacon...
void release_ifconfig(unsigned char* ifconfig)
{
	if (ifconfig != NULL)
	{
		free(ifconfig);
	}

}

struct interface *int_list = NULL, *int_last = NULL;

void free_interface_list( void );


static struct interface *add_interface(char *name)
{
    struct interface *ife, **nextp, *new;

	DLX(9, printf( "Entering add_interface()\n"));

    for (ife = int_last; ife; ife = ife->prev) {
		DLX(9, printf("add_interface() string compare loop\n"));
        int n = strcmp(ife->name, name);

        if (n == 0)		// already in list
		{
			DLX(9, printf("interface (%s) already in list\n", name));
            return ife;
		}
        else			// new interface, not already in list
		{
			DLX(9, printf("new interface (%s), add to list\n", name));
            break;
		}
    }

//	D( printf( " DEBUG: add_interface(), Line: %i\n", __LINE__ ); )

	new = calloc( 1, sizeof( struct interface ) );

	if ( new == NULL )
	{
		DLX(9, perror("calloc()"));
		return ( ( struct interface *)NULL );
	}

//	D( printf( " DEBUG: add_interface(), Line: %i\n", __LINE__ ); )

    strncpy( new->name, name, IFNAMSIZ );

    nextp = ife ? &ife->next : &int_list;
    new->prev = ife;
    new->next = *nextp;

    if (new->next)
        new->next->prev = new;
    else
        int_last = new;

    *nextp = new;

	DLX(9, printf("returning from add_interface() okay\n"));
    return new;
}


char * skip_whitespace(const char *s)
{
    /* In POSIX/C locale (the only locale we care about: do we REALLY want
     * to allow Unicode whitespace in, say, .conf files? nuts!)
     * isspace is only these chars: "\t\n\v\f\r" and space.
     * "\t\n\v\f\r" happen to have ASCII codes 9,10,11,12,13.
     * Use that.
     */
    while (*s == ' ' || (unsigned char)(*s - 9) <= (13 - 9))
        s++;

    return (char *) s;
}


# define isspace(c) ((c) == ' ')

static char *get_name(char *name, char *p)
{
    /* Extract <name> from nul-terminated p where p matches
     * <name>: after leading whitespace.
     * If match is not made, set name empty and return unchanged p
     */
    char *nameend;
    char *namestart = skip_whitespace(p);

    nameend = namestart;
    while (*nameend && *nameend != ':' && !isspace(*nameend))
        nameend++;
    if (*nameend == ':') {
        if ((nameend - namestart) < IFNAMSIZ) {
            memcpy(name, namestart, nameend - namestart);
            name[nameend - namestart] = '\0';
            p = nameend;
        } else {
            /* Interface name too large */
            name[0] = '\0';
        }
    } else {
        /* trailing ':' not found - return empty */
        name[0] = '\0';
    }
    return p + 1;
}

static int if_readlist_proc( char *target )
{
//	static int			proc_read;
	int					proc_read;
	FILE				*fh;
	char				buf[512];
	struct interface	*ife;
//	int					rv; //, procnetdev_vsn;

	// if list already built, don't build again
//	if ( proc_read ) return 0;

	if ( !target ) proc_read = 1;

	DLX(9, printf("opening %s for reading\n", _PATH_PROCNET_DEV));
	fh = fopen( _PATH_PROCNET_DEV, "r" );
	if ( fh == NULL )
	{
		D( perror( " fopen()" ); )
		return -1;
	}
//	TODO:	if ( !fh ) return if_readconf(); 

// TODO: add error handling of fgets()
	DLX(9, printf("reading first line of %s\n", _PATH_PROCNET_DEV));
	fgets( buf, sizeof buf, fh );	/* eat line */
	DLX(9, printf("reading second line of %s\n", _PATH_PROCNET_DEV));
	fgets( buf, sizeof buf, fh );	/* eat line */

//	procnetdev_vsn = procnetdev_version( buf );

	while ( fgets( buf, sizeof buf, fh ) ) {
		char *s, name[128];
		memset( name, 0, 128 );
		s = get_name( name, buf );
		DLX(9, printf("found interface %s\n", name));
		if ( ( ife = add_interface( name ) ) == NULL ) goto ERROR;
	}

	if ( ferror( fh ) ) goto ERROR;

	fclose( fh );
	DLX(9, printf("DEBUG: returning from if_readlist_proc()\n"));
	return 0;

ERROR:
//	proc_read = 0;
	printf( " error in reading %s\n", _PATH_PROCNET_DEV );
	free_interface_list();
	fclose( fh );
	return -1;
}

static int if_readlist( void )
{
	int		rv;

	DLX(9, printf("starting if_readlist()\n"));
	rv = if_readlist_proc( NULL );

	// TODO: if error, fall back to reading interfaces via ioctl()
/*
	if (!rv)
	{
		rv = if_readconf( void );
	}
*/
	DLX(9, printf("returning from if_readlist()\n"));
	return rv;
}

/* Fetch the interface configuration from the kernel. */
static int if_fetch( struct interface *ife )
{
    struct ifreq	ifr;
    char			*ifname = ife->name;
    int				skfd;

    if ( ( skfd = socket(AF_INET, SOCK_DGRAM, 0) ) < 0 ) return -1;

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
        close(skfd);
        return -1;
    }
    ife->flags = ifr.ifr_flags;

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    memset(ife->hwaddr, 0, 32);
    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0)
        memcpy(ife->hwaddr, ifr.ifr_hwaddr.sa_data, 8);

    ife->type = ifr.ifr_hwaddr.sa_family;

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ife->metric = 0;
    if (ioctl(skfd, SIOCGIFMETRIC, &ifr) >= 0)
        ife->metric = ifr.ifr_metric;

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ife->mtu = 0;
    if (ioctl(skfd, SIOCGIFMTU, &ifr) >= 0)
        ife->mtu = ifr.ifr_mtu;

    memset(&ife->map, 0, sizeof(struct ifmap));
#ifdef SIOCGIFMAP
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(skfd, SIOCGIFMAP, &ifr) == 0)
        ife->map = ifr.ifr_map;
#endif

#ifdef HAVE_TXQUEUELEN
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ife->tx_queue_len = -1; /* unknown value */
    if (ioctl(skfd, SIOCGIFTXQLEN, &ifr) >= 0)
        ife->tx_queue_len = ifr.ifr_qlen;
#else
    ife->tx_queue_len = -1; /* unknown value */
#endif

/* pull IPv4 addresses */
    ifr.ifr_addr.sa_family = AF_INET;
    memset(&ife->addr, 0, sizeof(struct sockaddr));
    if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0) {
        ife->has_ip = 1;
        ife->addr = ifr.ifr_addr;
    	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        memset(&ife->dstaddr, 0, sizeof(struct sockaddr));
        if (ioctl(skfd, SIOCGIFDSTADDR, &ifr) >= 0)
            ife->dstaddr = ifr.ifr_dstaddr;

    	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        memset(&ife->broadaddr, 0, sizeof(struct sockaddr));
        if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) >= 0)
            ife->broadaddr = ifr.ifr_broadaddr;

    	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        memset(&ife->netmask, 0, sizeof(struct sockaddr));
        if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0)
            ife->netmask = ifr.ifr_netmask;
    }

    close(skfd);

    return 0;
}


static int do_if_fetch( struct interface *ife )
{
	char tempString[1024];
	char *oldOutput = NULL;

    if ( if_fetch( ife ) < 0 ) {
        const char *errmsg;

        if ( errno == ENODEV ) {
            /* Give better error message for this case. */
            errmsg = "Device not found";
        } else {
            errmsg = strerror(errno);
        }

        //printf( "%s: error fetching interface information: %s\n", ife->name, errmsg);
		//snprintf(tempString, 1000, "%s: error fetching interface information: %s\n", ife->name, errmsg);
		memset( tempString, '\0', sizeof(tempString) );	
		snprintf(tempString, sizeof(tempString) - 1, "%s: %s %s\n", ife->name, ifc1, errmsg);
	
		//Concatenate to iOutputBuffer and expand it if necessary...
		if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 		{
			if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
			sizeIncrementCount++;
			iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
			memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
			if (oldOutput != NULL)
 			{
				strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
				free(oldOutput);
				oldOutput = NULL;
			}
		}
		strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);

        return -1;
    }

    return 0;
}

static void ife_print6(struct interface *ptr)
{
	char tempString[1024];
	char *oldOutput = NULL;

    FILE *f;
    char addr6[40], devname[20];
    struct sockaddr_in6 sap;
    int plen, scope, dad_status, if_idx;
    char addr6p[8][5];

    f = fopen(_PATH_PROCNET_IFINET6, "r");
    if (f == NULL)
        return;

    while (fscanf
           (f, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
            addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
            addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope,
            &dad_status, devname) != EOF
    ) {
        if (!strcmp(devname, ptr->name)) {
            sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
                    addr6p[0], addr6p[1], addr6p[2], addr6p[3],
                    addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
            inet_pton(AF_INET6, addr6,
                      (struct sockaddr *) &sap.sin6_addr);
            sap.sin6_family = AF_INET6;
            //printf("  inet6 addr:\t%s/%d", sock_ntop_host((struct sockaddr *) &sap, sizeof( struct sockaddr_in6 ) ), plen);
            //snprintf( tempString, 1000, "  inet6 addr:\t%s/%d", sock_ntop_host((struct sockaddr *) &sap, sizeof( struct sockaddr_in6 ) ), plen);
			memset( tempString, '\0', sizeof(tempString) );
            snprintf( tempString, sizeof(tempString) - 1, "  %s:\t%s/%d", ifc2, sock_ntop_host((struct sockaddr *) &sap, sizeof( struct sockaddr_in6 ) ), plen);
	
			//Concatenate to iOutputBuffer and expand it if necessary...
			if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 			{
				if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
				sizeIncrementCount++;
				iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
				memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
				if (oldOutput != NULL)
 				{
					strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
					free(oldOutput);
					oldOutput = NULL;
				}
			}
			strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);

            //printf("   \t<Scope:");
            //snprintf(tempString, 1000, "   \t<Scope:");
			memset( tempString, '\0', sizeof(tempString) );
            snprintf(tempString, sizeof(tempString) - 1, "   \t<%s:", ifc3);
	
			//Concatenate to iOutputBuffer and expand it if necessary...
			if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 			{
				if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
				sizeIncrementCount++;
				iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
				memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
				if (oldOutput != NULL)
 				{
					strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
					free(oldOutput);
					oldOutput = NULL;
				}
			}
			strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
    
			switch (scope & IPV6_ADDR_SCOPE_MASK) {
            	case 0:
                	//puts("Global>");
					//snprintf(tempString, 1000, "Global>");
					memset( tempString, '\0', sizeof(tempString) );
					snprintf(tempString, sizeof(tempString) - 1, "%s>", ifc4);
	
					//Concatenate to iOutputBuffer and expand it if necessary...
					if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 					{
						if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
						sizeIncrementCount++;
						iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
						memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
						if (oldOutput != NULL)
 						{
							strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
							free(oldOutput);
							oldOutput = NULL;
						}
					}
					strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
                	break;
            	case IPV6_ADDR_LINKLOCAL:
                	//puts("Link>");
					//snprintf(tempString, 1000, "Link>");
					memset( tempString, '\0', sizeof(tempString) );
					snprintf(tempString, sizeof(tempString) - 1, "%s>", ifc5);
	
					//Concatenate to iOutputBuffer and expand it if necessary...
					if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 					{
						if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
						sizeIncrementCount++;
						iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
						memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
						if (oldOutput != NULL)
 						{
							strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
							free(oldOutput);
							oldOutput = NULL;
						}
					}
					strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
                	break;
            	case IPV6_ADDR_SITELOCAL:
                	//puts("Site>");
					//snprintf(tempString, 1000, "Site>");
					memset( tempString, '\0', sizeof(tempString) );
					snprintf(tempString, sizeof(tempString) - 1, "%s>", ifc6);
	
					//Concatenate to iOutputBuffer and expand it if necessary...
					if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 					{
						if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
						sizeIncrementCount++;
						iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
						memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
						if (oldOutput != NULL)
 						{
							strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
							free(oldOutput);
							oldOutput = NULL;
						}
					}
					strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
                	break;
            	case IPV6_ADDR_COMPATv4:
                	//puts("Compat>");
					//snprintf(tempString, 1000, "Compat>");
					memset( tempString, '\0', sizeof(tempString) );
					snprintf(tempString, sizeof(tempString) - 1, "%s>", ifc7);
	
					//Concatenate to iOutputBuffer and expand it if necessary...
					if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 					{
						if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
						sizeIncrementCount++;
						iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
						memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
						if (oldOutput != NULL)
 						{
							strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
							free(oldOutput);
							oldOutput = NULL;
						}
					}
					strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
                	break;
            	case IPV6_ADDR_LOOPBACK:
                	//puts("Host>");
					//snprintf(tempString, 1000, "Host>");
					memset( tempString, '\0', sizeof(tempString) );
					snprintf(tempString, sizeof(tempString) - 1, "%s>", ifc8);
	
					//Concatenate to iOutputBuffer and expand it if necessary...
					if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 					{
						if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
						sizeIncrementCount++;
						iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
						memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
						if (oldOutput != NULL)
 						{
							strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
							free(oldOutput);
							oldOutput = NULL;
						}
					}
					strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
                	break;
            	default:
                	//puts("Unknown>");
					//snprintf(tempString, 1000, "Unknown>");
					memset( tempString, '\0', sizeof(tempString) );
					snprintf(tempString, sizeof(tempString) - 1, "%s>", ifc9);
	
					//Concatenate to iOutputBuffer and expand it if necessary...
					if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 					{
						if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
						sizeIncrementCount++;
						iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
						memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
						if (oldOutput != NULL)
 						{
							strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
							free(oldOutput);
							oldOutput = NULL;
						}
					}
					strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
		
			//Concatenate to iOutputBuffer and expand it if necessary...
			if ( strlen("\n") + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 			{
				if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
				sizeIncrementCount++;
				iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
				memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
				if (oldOutput != NULL)
 				{
					strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
					free(oldOutput);
					oldOutput = NULL;
				}	
			}
			strncat( iOutputBuffer, "\n", sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);

			} //end of switch (scope& IPV6_ADDR_SCOPE_MASK

		}   //End of if (!strcmp(devname, ptr->name))
	}
    fclose(f);
}

static void ife_print( struct interface *ifi )
{
	char tempString[1024];
	char			*ptr;
	char status[256];
	char *oldOutput = NULL;

    //printf("%s: ", ifi->name);
	memset( tempString, '\0', sizeof(tempString) );
    snprintf(tempString, sizeof(tempString) - 1, "%s: ", ifi->name);
    //printf("<");
    strncat(tempString, "<", sizeof(tempString) - strlen(tempString) - 1);

	//Clear Status
	memset( status, '\0', sizeof(status) );

/* *INDENT-OFF* */
    //if (ifi->flags & IFF_UP)            strncat( tempString, "UP ", 1000);   //printf("UP ");
    if (ifi->flags & IFF_UP)
  	{
  		snprintf( status, sizeof(status) - 1, "%s", ifc10);
  		strncat( tempString, status, sizeof(tempString) - strlen(tempString) -1 );   //printf("UP ");
 	}
    //if (ifi->flags & IFF_LOOPBACK)      strncat( tempString, "LOOP ", 1000);   //printf("LOOP ");
	if (ifi->flags & IFF_LOOPBACK)
	{
  		snprintf( status, sizeof(status) - 1, "%s", ifc11);
  		strncat( tempString, status, sizeof(tempString) - strlen(tempString) -1);   //printf("LOOP ");
 	}
    //if (ifi->flags & IFF_BROADCAST)     strncat( tempString, "BCAST ", 1000);   //printf("BCAST ");
	if (ifi->flags & IFF_BROADCAST)
	{
  		snprintf( status, sizeof(status) - 1, "%s", ifc12);
  		strncat( tempString, status, sizeof(tempString) - strlen(tempString) -1);   //printf("BCAST ");
 	}
    //if (ifi->flags & IFF_RUNNING)    	  strncat( tempString, "RUNNING ", 1000);   //printf("RUNNING ");
	if (ifi->flags & IFF_RUNNING)
	{
  		snprintf( status, sizeof(status) - 1, "%s", ifc13);
  		strncat( tempString, status, sizeof(tempString) - strlen(tempString) -1);   //printf("RUNNING ");
 	}
    //if (ifi->flags & IFF_MULTICAST)     strncat( tempString, "MCAST ", 1000);   //printf("MCAST ");
	if (ifi->flags & IFF_MULTICAST)
	{
  		snprintf( status, sizeof(status)- 1, "%s", ifc14);
  		strncat( tempString, status, sizeof(tempString) - strlen(tempString) -1);   //printf("MCAST ");
 	}
    //if (ifi->flags & IFF_NOARP)         strncat( tempString, "NOARP ", 1000);   //printf("NOARP ");
	if (ifi->flags & IFF_NOARP)
 	{
  		snprintf( status, sizeof(status) - 1, "%s", ifc15);
  		strncat( tempString, status, sizeof(tempString) - strlen(tempString) -1);   //printf("NOARP ");
 	}
    //if (ifi->flags & IFF_PROMISC)       strncat( tempString, "PROMISC ", 1000);   //printf("PROMISC ");
	if (ifi->flags & IFF_PROMISC)
 	{
  		snprintf( status, sizeof(status) - 1, "%s", ifc16);
  		strncat( tempString, status, sizeof(tempString) - strlen(tempString) -1);   //printf("PROMISC ");
 	}
	//if (ifi->flags & IFF_POINTOPOINT)   strncat( tempString, "P2P ", 1000);   //printf("P2P ");
	if (ifi->flags & IFF_POINTOPOINT)
 	{
  		snprintf( status, sizeof(status) - 1, "%s", ifc17);
  		strncat( tempString, status, sizeof(tempString) - strlen(tempString) -1);   //printf("P2P ");
 	}
    //if (ifi->flags & IFF_DYNAMIC)       strncat( tempString, "DYNAMIC ", 1000);   //printf("DYNAMIC ");
	if (ifi->flags & IFF_DYNAMIC)
 	{
  		snprintf( status, sizeof(status) - 1, "%s", ifc18);
  		strncat( tempString, status, sizeof(tempString) - strlen(tempString) - 1);   //printf("DYNAMIC ");
 	}
    strncat( tempString, ">\n", sizeof(tempString) - strlen(tempString) -1);   //printf(">\n");
/* *INDENT-ON* */

	//Concatenate to iOutputBuffer and expand it if necessary...
	if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 	{
		if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
		sizeIncrementCount++;
		iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
		memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
		if (oldOutput != NULL)
		{
			strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
			free(oldOutput);
			oldOutput = NULL;
		}
	}
	strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);

	ptr = ifi->hwaddr;
    //printf("  HW addr:\t");
    //snprintf(tempString, 1000, "  HW addr:\t");
	memset( tempString, '\0', sizeof(tempString) );
	snprintf(tempString, sizeof(tempString) - 1, "  %s:\t", ifc19);

	//Concatenate to iOutputBuffer and expand it if necessary...
	if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 	{
		if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
		sizeIncrementCount++;
		iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
		memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
		if (oldOutput != NULL)
 		{
			strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
			free(oldOutput);
			oldOutput = NULL;
		}
	}
	strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);

	//printf("%02X-%02X-%02X-%02X-%02X-%02X",
    //       (ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
    //       (ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377) );
    //   printf("\n");
	memset( tempString, '\0', sizeof(tempString) );
	snprintf(tempString, sizeof(tempString) - 1, "%02X-%02X-%02X-%02X-%02X-%02X\n",
             (ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
             (ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377) );

	//Concatenate to iOutputBuffer and expand it if necessary...
	if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 	{
		if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
		sizeIncrementCount++;
		iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
		memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
		if (oldOutput != NULL)
 		{
			strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
			free(oldOutput);
			oldOutput = NULL;
		}
	}
	strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);

    if (ifi->mtu != 0)
	{
        //printf("  MTU:\t\t%d\n", ifi->mtu);
        //snprintf(tempString, 1000, "  MTU:\t\t%d\n", ifi->mtu);
		memset( tempString, '\0', sizeof(tempString) );
        snprintf(tempString, sizeof(tempString) - 1, "  %s:\t\t%d\n", ifc20, ifi->mtu);

		//Concatenate to iOutputBuffer and expand it if necessary...
		if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
 		{
			if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
			sizeIncrementCount++;
			iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
			memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
			if (oldOutput != NULL)
 			{
				strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
				free(oldOutput);
				oldOutput = NULL;
			}
		}
		strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
	}

	if (ifi->has_ip) 
	{
        //printf("  inet addr:\t%s\n", sock_ntop_host( &(ifi->addr), sizeof(ifi->addr)));
        //snprintf(tempString, 1000, "  inet addr:\t%s\n", sock_ntop_host( &(ifi->addr), sizeof(ifi->addr)));
		memset( tempString, '\0', sizeof(tempString) );
        snprintf(tempString, sizeof(tempString) - 1, "  %s:\t%s\n", ifc21, sock_ntop_host( &(ifi->addr), sizeof(ifi->addr)));

		//Concatenate to iOutputBuffer and expand it if necessary...
		if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
        {
			if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
            sizeIncrementCount++;
            iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
            memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
            if (oldOutput != NULL)
            {
				strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
				free(oldOutput);
				oldOutput = NULL;
			}
        }
		strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);

		//printf("  bcast addr:\t%s\n", sock_ntop_host( &(ifi->broadaddr), sizeof(ifi->broadaddr)));
        //snprintf(tempString, 1000, "  bcast addr:\t%s\n", sock_ntop_host( &(ifi->broadaddr), sizeof(ifi->broadaddr)));
		memset( tempString, '\0', sizeof(tempString) );
        snprintf(tempString, sizeof(tempString) - 1, "  %s:\t%s\n", ifc22, sock_ntop_host( &(ifi->broadaddr), sizeof(ifi->broadaddr)));

		//Concatenate to iOutputBuffer and expand it if necessary...
		if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
        {
			if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
            sizeIncrementCount++;
            iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
            memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
            if (oldOutput != NULL)
            {
				strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
				free(oldOutput);
				oldOutput = NULL;
			}
        }
		strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
            
		//printf("  netmask:\t%s\n", sock_ntop_host( &(ifi->netmask), sizeof(ifi->dstaddr)));
        //snprintf(tempString, 1000, "  netmask:\t%s\n", sock_ntop_host( &(ifi->netmask), sizeof(ifi->dstaddr)));
		memset( tempString, '\0', sizeof(tempString) );
        snprintf(tempString, sizeof(tempString) - 1, "  %s:\t%s\n", ifc23, sock_ntop_host( &(ifi->netmask), sizeof(ifi->dstaddr)));

		//Concatenate to iOutputBuffer and expand it if necessary...
		if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
        {
			if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
            sizeIncrementCount++;
            iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
            memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
            if (oldOutput != NULL)
            {
				strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
				free(oldOutput);
				oldOutput = NULL;
			}
        }
		strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);


		if ( ifi->flags & IFF_POINTOPOINT )
		{
       	    //printf("  P-t-P addr:\t%s\n", sock_ntop_host( &(ifi->dstaddr), sizeof(ifi->dstaddr)));
       	    //snprintf(tempString, 1000, "  P-t-P addr:\t%s\n", sock_ntop_host( &(ifi->dstaddr), sizeof(ifi->dstaddr)));
			memset( tempString, '\0', sizeof(tempString) );
       	    snprintf(tempString, sizeof(tempString) - 1, "  %s:\t%s\n", ifc24, sock_ntop_host( &(ifi->dstaddr), sizeof(ifi->dstaddr)));

			//Concatenate to iOutputBuffer and expand it if necessary...
			if ( strlen(tempString) + 1 > sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) )
       		{
				if (iOutputBuffer != NULL) oldOutput = iOutputBuffer;
           		sizeIncrementCount++;
           		iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
           		memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
           		if (oldOutput != NULL)
           		{
					strncat( iOutputBuffer, oldOutput, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
					free(oldOutput);
					oldOutput = NULL;
				}
       		}
			strncat( iOutputBuffer, tempString, sizeIncrementCount*defaultOutputSize - strlen(iOutputBuffer) - 1);
		}
	}

	ife_print6( ifi );
	return;
}


static int do_if_print(struct interface *ife) /*, int *opt_a)*/
{
    int res;

    res = do_if_fetch(ife);
    if (res >= 0) {
//        if ( (ife->flags & IFF_UP) || interface_opt_a)
            ife_print(ife);
    }
    return res;
}

void free_interface_list( void )
{

	D( int i = 0; )
	struct interface *ife_prev;

	while ( int_last != int_list ) {
		D( i++; )
		// save the pointer the next interface before we lose it (i.e. free it )
		ife_prev = int_last->prev;
		DLX(9, printf("freeing %s interface\n", int_last->name));
		if ( int_last != NULL ) free( int_last );
		int_last = ife_prev;
	};

	if ( int_list != NULL )
	{
		D( i++; )
		DLX(9, printf("freeing %s interface (int_list)\n", int_list->name));
		free( int_list );
		int_list = NULL;
		int_last = NULL;
	}

	DLX(9, printf("%d interfaces freed\n", i));
	return;
}

char * sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];		/* Unix domain is largest */
	memset( str, '\0', sizeof(str) );

	switch (sa->sa_family) {
		case AF_INET: {
			struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
		}

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}
#endif

	default: {
		//snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
		//		 sa->sa_family, salen);
		snprintf(str, sizeof(str), "%s: %d, len %d",
				 ifc25, sa->sa_family, salen);
		return(str);
	
	}
	}
	return (NULL);
}


//int get_ifconfig(unsigned char* buf, int* size)
unsigned char* get_ifconfig( int* size)
{
	*size = 0;    

	int outputLength = 0;

    struct interface  *ife;
	int               rv;
	iOutputBuffer = NULL;
	sizeIncrementCount = 1;

	//Initialize iOutputBuffer...
	iOutputBuffer = (char *) malloc( sizeIncrementCount*defaultOutputSize );
	memset( iOutputBuffer, '\0', sizeIncrementCount*defaultOutputSize );
	strncat(iOutputBuffer, "\n", sizeIncrementCount*defaultOutputSize-1);  //starts the beacon data off with a new line...
 
	int_last = NULL;
	int_list = NULL;

	DLX(9, printf("starting get_ifconfig()\n"));
   	rv = if_readlist();
	if ( rv < 0 )
	{
		DLX(9, printf ("ERROR: if_readlist() failed. exiting\n"));
	   return NULL;
   	}


   	DLX(9, printf ("get_ifconfig(): do_if_print()\n"));
   	for ( ife = int_list; ife; ife = ife-> next ) {
       rv = do_if_print( ife );
       if ( rv < 0 )
       {
			if (iOutputBuffer != NULL) free(iOutputBuffer);
			iOutputBuffer=0;
       		DLX(9, printf("get_ifconfig(): do_if_print() fail\n"));
       		return NULL;
       }
   }

   DLX(9, printf("get_ifconfig(): do_if_print() success\n"));
   free_interface_list();

	outputLength= strlen(iOutputBuffer) + 1;

	//New code to exit
	if (outputLength > 1)
	{
		*size = sizeIncrementCount * defaultOutputSize;
		return (unsigned char *) iOutputBuffer;	
		
	}
	else
	{
		*size = 0;
		if (iOutputBuffer != NULL) free(iOutputBuffer);
		return NULL;

	}

}

//#include	"unp.h"
#endif
