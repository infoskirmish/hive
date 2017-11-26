#ifndef _DIESELT_NETWORK_H
#define _DIESELT_NETWORK_H

#include <stdlib.h>
#include <stdint.h>
#include "debug.h"

typedef struct udpHDR_type {
	uint16_t sPort;
	uint16_t dPort;
	uint16_t len;
	uint16_t check;
} udp_hdr;

typedef struct ethHDR_type {
	uint8_t dest[6];
	uint8_t src[6];
	uint16_t type;
} eth_hdr;

typedef struct ipHDR_type {
	uint8_t ver_len;
	uint8_t tos;
	uint16_t tLen;
	uint16_t id;
	uint16_t f_offset;
	uint8_t ttl;
	uint8_t proto;
	uint16_t chksum;
	uint32_t src;
	uint32_t dest;
} ip_hdr;

typedef struct icmpHDR_type {
	uint8_t type;
	uint8_t code;
	uint16_t chksum;
	uint16_t id;
	uint16_t seq;
} icmp_hdr;


extern void send_RAW_from_IPPROTO(uint32_t addr, uint8_t * wiredata, size_t len);

extern int send_UDP_data(uint32_t s_addr, uint32_t d_addr, uint16_t s_port, uint16_t d_port, uint8_t * udp_data, size_t len);

extern int send_TCP_data(uint32_t s_addr, uint32_t d_addr, uint16_t s_port, uint16_t d_port, uint8_t * tcp_data, size_t len);


extern uint16_t cksum(uint16_t * buf, uint32_t sz);

#endif
