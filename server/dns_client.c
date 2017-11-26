#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "dns_protocol.h"
#include "decode_dns.h"
#include "debug.h"

/*!
 * @brief Perform DNS lookup
 *
 * 		See RFC 1035 section 4 for DNS message details.
 *
 * @param ip - IP address or domain name of host
 * @param serverIP - DNS server IP address (e.g. "192.168.1.53")
 * @returns a pointer to the dotted quad address string (malloc'd memory that must be freed)
 */
char *dns_resolv(char *ip, char *serverIP)
{
	uint8_t buf[MAX_MSG_LENGTH] = {0};
	DNS_header *header;
	DNS_response *response;

	int sock;
	struct sockaddr_in sin;
	int sin_len = sizeof(sin);

	struct timeval timer;
	void *qp;
	size_t buflen = 0;
	char *p;
	int n;
	uint16_t queryID = 0;

	timer.tv_sec = DNS_TIMEOUT;
	timer.tv_usec = 0;

	DLX(5,printf("Attempting to resolve %s\n", ip));
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		DLX(4, perror("Could not create socket"));
		return NULL;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timer, sizeof(struct timeval)) < 0) {
		DLX(4, printf("Failed to set timeout on socket: %s\n", strerror(errno)));
	}
	memset((char *) &sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(53);							// DNS UDP port number
	inet_aton(serverIP, &sin.sin_addr);					// DNS server address

	// DNS Header Initialization

	header = (DNS_header *)buf;
	queryID = htons((uint16_t)rand());
	header->id = htons(queryID);						// Randomly generated query ID
	header->qdcount = htons(1);							// One query
	header->rd = 1;										// Set recursion flag
	header->flags = htons(header->flags);

	// Generate the query
	{
		char *tbuf;	// Pointer to temporary buffer for parsing domain name
		D(char *x;)

		if ((tbuf = calloc(strlen(ip)+1, 1)) ==  NULL) {	// Create temporary buffer
			close(sock);
			return NULL;
		}

		memcpy(tbuf, ip, strlen(ip));
		qp = (char *) (buf + sizeof(DNS_header));		// Skip over header and build DNS formatted name
		D(x = qp;)
		p = strtok(tbuf, ".");							// p points to first part of name
		while (p) {
			*((uint8_t *)qp++) = (uint8_t)strlen(p);	// Add length encoding
			memcpy((char *)qp, p, strlen(p));			// Copy portion of name
			qp += strlen(p);							// Reposition pointer to next part of name
			p = strtok(NULL, ".");						// Repeat until entire domain name encoded
		}
		*(char *)qp++ = '\0';							// Null byte terminates Qname field
		DLX(5, printf("Query Buffer: %s\n", x));
		*(uint16_t *)qp = htons(1), qp += 2;			//  Qtype = 1 (A record)
		*(uint16_t *)qp++ = htons(1), qp += 2;			// Qclass = 1 (IN ==> INTERNET)
		free(tbuf);
	}
	// Send DNS query
	DLX(5,printf("Sending DNS query using socket: %d ...\n", sock));
	buflen = (size_t)qp - (size_t)buf;
	DPB(6, "DNS Query: ", buf, buflen);
	n = sendto(sock, buf, buflen, 0, (struct sockaddr *) &sin, sin_len);
	if (n < 0) {
		close(sock);
		DLX(4, perror("Could not send DNS query"));
		return NULL;
	}

//*************************************************************************************************

	// Wait for DNS server response

	response = (DNS_response *)buf;
	DLX(4,printf("Waiting for response from DNS server...\n"));
	do {
		n = recv(sock, buf, MAX_MSG_LENGTH, 0);
		if (n < 0) {
			if (errno == EAGAIN)
				continue;
			close(sock);
			return NULL;
		}
		if (n < (int)sizeof(DNS_header))					// Must at least see a DNS-sized header
			DLX(4, printf("Packet received is %i bytes -- too small for a DNS response\n", n));
			continue;
		header = (DNS_header *)buf;
		DL(5);
	} while (ntohs(header->id) != queryID && !header->qr);		// QR must be set and the header ID must match the queryID
	close(sock);

	if (header->ancount == 0) {
		DLX(4, printf("%s did not resolve\n", ip));
		return NULL;
	}

	return (decode_dns(response));
}
