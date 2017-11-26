#ifndef __DNS_PROTOCOL_H__
#define __DNS_PROTOCOL_H__

#include <sys/types.h>

#define MAX_MSG_LENGTH	512
#define	DNS_TIMEOUT		30		// 30-second timeout

/* QR
 * a one bit field that specifies whether this message is a query
 * or a response
 */
#define QR_QUERY 0
#define QR_RESPONSE 1

/* OPCODE
 * a four bit field that specifies kind of query in this message
 */
#define OPCODE_QUERY 0 /* a standard query */
#define OPCODE_IQUERY 1 /* an inverse query */
#define OPCODE_STATUS 2 /* a server status request */
/* 3-15 reserved for future use */

/* AA
 * one bit, valid in responses, and specifies that the responding
 * name server is an authority for the domain name in question
 * section
 */
#define AA_NONAUTHORITY 0
#define AA_AUTHORITY 1

typedef struct {
	u_int16_t id; /* a 16 bit identifier assigned by the client */
	union {
		uint16_t flags;
		struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
			u_int16_t rcode:4;
			u_int16_t z:3;
			u_int16_t ra:1;
			u_int16_t rd:1;
			u_int16_t tc:1;
			u_int16_t aa:1;
			u_int16_t opcode:4;
			u_int16_t qr:1;
#elif __BYTE_ORDER ==__BIG_ENDIAN
			u_int16_t qr:1;
			u_int16_t opcode:4;
			u_int16_t aa:1;
			u_int16_t tc:1;
			u_int16_t rd:1;
			u_int16_t ra:1;
			u_int16_t z:3;
			u_int16_t rcode:4;
#else
#error Endian undefined
#endif
		} __attribute__((packed));
	};
	u_int16_t qdcount;
	u_int16_t ancount;
	u_int16_t nscount;
	u_int16_t arcount;
} DNS_header;

#if 0
typedef struct {
	DNS_header header;
	union
//	struct dns_question question;
	char *data;
	u_int16_t data_size;
} DNS_packet;
#endif

#define A_RECORD	1
typedef struct {
	char *name;
	u_int16_t type;
	u_int16_t class;
	u_int32_t ttl;
	u_int16_t rdlength;
	char *rdata;
} DNS_response;

typedef struct {
	char *qname;
	u_int16_t qtype;
	u_int16_t qclass;
} DNS_query;

char *dns_resolv(char *ip, char *serverIP);
void timeout_handler(int signal);
#endif
