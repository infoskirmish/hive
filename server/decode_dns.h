#ifndef _DECODE_DNS_
#define _DECODE_DNS_
#include <stdint.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "debug.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
// DNS
///////
// Definitions in this section were taken from /usr/include/arpa/nameser.h and modified as needed.
// Many of the definitions found here may not actually be used.

/*
 * Define constants based on RFC 883, RFC 1034, RFC 1035
 */
#define NS_PACKETSZ	512	/* default UDP packet size */
#define NS_MAXDNAME	1025	/* maximum domain name */
#define NS_MAXMSG	65535	/* maximum message size */
#define NS_MAXCDNAME	255	/* maximum compressed domain name */
#define NS_MAXLABEL	63	/* maximum length of domain label */
#define NS_HFIXEDSZ	12	/* #/bytes of fixed data in header */
#define NS_QFIXEDSZ	4	/* #/bytes of fixed data in query */
#define NS_RRFIXEDSZ	10	/* #/bytes of fixed data in r record */
#define NS_INT32SZ	4	/* #/bytes of data in a uint32_t */
#define NS_INT16SZ	2	/* #/bytes of data in a uint16_t */
#define NS_INT8SZ	1	/* #/bytes of data in a uint8_t */
#define NS_INADDRSZ	4	/* IPv4 T_A */
#define NS_IN6ADDRSZ	16	/* IPv6 T_AAAA */
#define NS_CMPRSFLGS	0xc0	/* Flag bits indicating name compression. */
#define NS_DEFAULTPORT	53	/* For both TCP and UDP. */

// Message header
typedef struct {
	uint16_t id;		/* query identification number */
	uint16_t flags;	/* flags */
	uint16_t qdcount;	/* number of question entries */
	uint16_t ancount;	/* number of answer entries */
	uint16_t nscount;	/* number of authority entries */
	uint16_t arcount;	/* number of resource entries */
} DNS_hdr;

/*
 * Macros for subfields of flag fields.
 */
#define	DNS_QR(np)	(ntohs((np)->flags) & 0x8000)		/* response flag */
#define	DNS_OPCODE(np)	((ntohs((np)->flags) >> 11) & 0xF)	/* purpose of message */
#define	DNS_AA(np)	(ntohs((np)->flags) & 0x0400)		/* authoritative answer */
#define	DNS_TC(np)	(ntohs((np)->flags) & 0x0200)		/* truncated message */
#define	DNS_RD(np)	(ntohs((np)->flags) & 0x0100)		/* recursion desired */

#define	DNS_RA(np)	(ntohs((np)->flags) & 0x80)		/* recursion available */
#define	DNS_AD(np)	(ntohs((np)->flags) & 0x20)		/* authentic data from named */
#define	DNS_CD(np)	(ntohs((np)->flags) & 0x10)		/* checking disabled by resolver */
#define	DNS_ZERO(np)	((ntohs((np)->flags) >> 4) & 0x7)	/* 3 bits should be zero */
#define	DNS_RCODE(np)	(ntohs((np)->flags) & 0xF)		/* response code */

// Resource record data structure
struct _dns_rr_data {
	uint16_t	type;
	uint16_t	rr_class;
	uint32_t	ttl;
	uint16_t	rdlength;
} __attribute__((packed));
typedef struct _dns_rr_data DNS_rr_data;

#if 0
// Broken code that isn't used anyway
typedef struct {
	char		name[NS_MAXDNAME];    // <--- this doesn't work here
	DNS_rr_data	rrmetadata;
	const u_char 	*rdata;
} DNS_rr;
#endif

/* Accessor macros - this is part of the public interface. */
#define ns_rr_name(rr)	(((rr).name[0] != '\0') ? (rr).name : ".")
#define ns_rr_type(rr)	((ns_type)((rr).type + 0))
#define ns_rr_class(rr)	((ns_class)((rr).rr_class + 0))
#define ns_rr_ttl(rr)	((rr).ttl + 0)
#define ns_rr_rdlen(rr)	((rr).rdlength + 0)
#define ns_rr_rdata(rr)	((rr).rdata + 0)

/*%
 * These don't have to be in the same order as in the packet flags word,
 * and they can even overlap in some cases, but they will need to be kept
 * in synch with ns_parse.c:ns_flagdata[].
 */
typedef enum __ns_flag {
	ns_f_qr,		/*%< Question/Response. */
	ns_f_opcode,		/*%< Operation code. */
	ns_f_aa,		/*%< Authoritative Answer. */
	ns_f_tc,		/*%< Truncation occurred. */
	ns_f_rd,		/*%< Recursion Desired. */
	ns_f_ra,		/*%< Recursion Available. */
	ns_f_z,			/*%< MBZ. */
	ns_f_ad,		/*%< Authentic Data (DNSSEC). */
	ns_f_cd,		/*%< Checking Disabled (DNSSEC). */
	ns_f_rcode,		/*%< Response code. */
	ns_f_max
} ns_flag;

/*%
 * Currently defined opcodes.
 */
typedef enum __ns_opcode {
	ns_o_query = 0,		/*%< Standard query. */
	ns_o_iquery = 1,	/*%< Inverse query (deprecated/unsupported). */
	ns_o_status = 2,	/*%< Name server status query (unsupported). */
				/* Opcode 3 is undefined/reserved. */
	ns_o_notify = 4,	/*%< Zone change notification. */
	ns_o_update = 5,	/*%< Zone update message. */
	ns_o_max = 6
} ns_opcode;

/*%
 * Currently defined response codes.
 */
typedef	enum __ns_rcode {
	ns_r_noerror = 0,	/*%< No error occurred. */
	ns_r_formerr = 1,	/*%< Format error. */
	ns_r_servfail = 2,	/*%< Server failure. */
	ns_r_nxdomain = 3,	/*%< Name error. */
	ns_r_notimpl = 4,	/*%< Unimplemented. */
	ns_r_refused = 5,	/*%< Operation refused. */
	/* these are for BIND_UPDATE */
	ns_r_yxdomain = 6,	/*%< Name exists */
	ns_r_yxrrset = 7,	/*%< RRset exists */
	ns_r_nxrrset = 8,	/*%< RRset does not exist */
	ns_r_notauth = 9,	/*%< Not authoritative for zone */
	ns_r_notzone = 10,	/*%< Zone of record different from zone section */
	ns_r_max = 11,
	/* The following are EDNS extended rcodes */
	ns_r_badvers = 16,
	/* The following are TSIG errors */
	ns_r_badsig = 16,
	ns_r_badkey = 17,
	ns_r_badtime = 18
} ns_rcode;


/*
 * Currently defined type values for resources and queries.
 */
typedef enum __ns_type {
	ns_t_invalid	= 0,	/* Cookie. */
	ns_t_a		= 1,	/* Host address. */
	ns_t_ns		= 2,	/* Authoritative server. */
	ns_t_md		= 3,	/* Mail destination. */
	ns_t_mf		= 4,	/* Mail forwarder. */
	ns_t_cname	= 5,	/* Canonical name. */
	ns_t_soa	= 6,	/* Start of authority zone. */
	ns_t_mb		= 7,	/* Mailbox domain name. */
	ns_t_mg		= 8,	/* Mail group member. */
	ns_t_mr		= 9,	/* Mail rename name. */
	ns_t_null	= 10,	/* Null resource record. */
	ns_t_wks	= 11,	/* Well known service. */
	ns_t_ptr	= 12,	/* Domain name pointer. */
	ns_t_hinfo	= 13,	/* Host information. */
	ns_t_minfo	= 14,	/* Mailbox information. */
	ns_t_mx		= 15,	/* Mail routing information. */
	ns_t_txt	= 16,	/* Text strings. */
	ns_t_rp		= 17,	/* Responsible person. */
	ns_t_afsdb	= 18,	/* AFS cell database. */
	ns_t_x25	= 19,	/* X_25 calling address. */
	ns_t_isdn	= 20,	/* ISDN calling address. */
	ns_t_rt		= 21,	/* Router. */
	ns_t_nsap	= 22,	/* NSAP address. */
	ns_t_nsap_ptr	= 23,	/* Reverse NSAP lookup (deprecated). */
	ns_t_sig	= 24,	/* Security signature. */
	ns_t_key	= 25,	/* Security key. */
	ns_t_px		= 26,	/* X.400 mail mapping. */
	ns_t_gpos	= 27,	/* Geographical position (withdrawn). */
	ns_t_aaaa	= 28,	/* Ip6 Address. */
	ns_t_loc	= 29,	/* Location Information. */
	ns_t_nxt	= 30,	/* Next domain (security). */
	ns_t_eid	= 31,	/* Endpoint identifier. */
	ns_t_nimloc	= 32,	/* Nimrod Locator. */
	ns_t_srv	= 33,	/* Server Selection. */
	ns_t_atma	= 34,	/* ATM Address */
	ns_t_naptr	= 35,	/* Naming Authority PoinTeR */
	ns_t_kx		= 36,	/* Key Exchange */
	ns_t_cert	= 37,	/* Certification record */
	ns_t_a6		= 38,	/* IPv6 address (deprecated, use ns_t_aaaa) */
	ns_t_dname	= 39,	/* Non-terminal DNAME (for IPv6) */
	ns_t_sink	= 40,	/* Kitchen sink (experimentatl) */
	ns_t_opt	= 41,	/* EDNS0 option (meta-RR) */
	ns_t_apl	= 42,	/* Address prefix list (RFC3123) */
	ns_t_tkey	= 249,	/* Transaction key */
	ns_t_tsig	= 250,	/* Transaction signature. */
	ns_t_ixfr	= 251,	/* Incremental zone transfer. */
	ns_t_axfr	= 252,	/* Transfer zone of authority. */
	ns_t_mailb	= 253,	/* Transfer mailbox records. */
	ns_t_maila	= 254,	/* Transfer mail agent records. */
	ns_t_any	= 255,	/* Wildcard match. */
	ns_t_zxfr	= 256,	/* BIND-specific, nonstandard. */
	ns_t_max	= 65536
} ns_type;

#define DNS_type(p)	(ntohs(((ns_query_typeclass *)p)->type))
typedef struct __ns_query_typeclass {
	uint16_t	type;
	uint16_t	class;
} ns_query_typeclass;
#endif

char *decode_dns(void *dns);
