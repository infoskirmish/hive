// Section 1: Everything but MikroTik
#if defined LINUX || defined SOLARIS
#ifndef __IFCONFIG_H
#define __IFCONFIG_H

#include "function_strings.h"

unsigned char* get_ifconfig(int* size);
void release_ifconfig(unsigned char* ifconfig);

#endif

#endif

// Section 2: MikroTik
// start of the MikroTik specific header
#if defined _IPCONFIG
/* Our own header for the programs that need interface configuration info.
   Include this file, instead of "unp.h". */

#ifndef	__unp_ifi_h
#define	__unp_ifi_h

#include "function_strings.h"
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>

//Beacon.c calls the next function to get the mikrotik router's ifconfig data...
//int get_ifconfig(unsigned char* buf, int* size);    //MikroTik ifconfig command...
unsigned char* get_ifconfig(int* size);
void release_ifconfig(unsigned char* ifconfig);

#define	IFI_NAME	16			/* same as IFNAMSIZ in <net/if.h> */
#define	IFI_HADDR	 8			/* allow for 64-bit EUI-64 in future */

struct user_net_device_stats {
    unsigned long long rx_packets;  /* total packets received       */
    unsigned long long tx_packets;  /* total packets transmitted    */
    unsigned long long rx_bytes;    /* total bytes received         */
    unsigned long long tx_bytes;    /* total bytes transmitted      */
    unsigned long rx_errors;    /* bad packets received         */
    unsigned long tx_errors;    /* packet transmit problems     */
    unsigned long rx_dropped;   /* no space in linux buffers    */
    unsigned long tx_dropped;   /* no space available in linux  */
    unsigned long rx_multicast; /* multicast packets received   */
    unsigned long rx_compressed;
    unsigned long tx_compressed;
    unsigned long collisions;

    /* detailed rx_errors: */
    unsigned long rx_length_errors;
    unsigned long rx_over_errors;   /* receiver ring buff overflow  */
    unsigned long rx_crc_errors;    /* recved pkt with crc error    */
    unsigned long rx_frame_errors;  /* recv'd frame alignment error */
    unsigned long rx_fifo_errors;   /* recv'r fifo overrun          */
    unsigned long rx_missed_errors; /* receiver missed packet     */
    /* detailed tx_errors */
    unsigned long tx_aborted_errors;
    unsigned long tx_carrier_errors;
    unsigned long tx_fifo_errors;
    unsigned long tx_heartbeat_errors;
    unsigned long tx_window_errors;
};

struct interface {
    struct interface *next, *prev;
    char name[IFNAMSIZ];                    /* interface name        */
    short type;                             /* if type               */
    short flags;                            /* various flags         */
    int metric;                             /* routing metric        */
    int mtu;                                /* MTU value             */
    int tx_queue_len;                       /* transmit queue length */
    struct ifmap map;                       /* hardware setup        */
    struct sockaddr addr;                   /* IP address            */
	struct sockaddr_in6 addr6;				/* IPv6 address			 */
    struct sockaddr dstaddr;                /* P-P IP address        */
    struct sockaddr broadaddr;              /* IP broadcast address  */
    struct sockaddr netmask;                /* IP network mask       */
    int has_ip;
    int has_ip6;
    char hwaddr[32];                        /* HW address            */
    int statistics_valid;
    struct user_net_device_stats stats;     /* statistics            */
    int keepalive;                          /* keepalive value for SLIP */
    int outfill;                            /* outfill value for SLIP */
};


#define	IFI_ALIAS	1			/* ifi_addr is an alias */

char * sock_ntop_host(const struct sockaddr *sa, socklen_t salen);

#endif	/* __unp_ifi_h */

#endif

