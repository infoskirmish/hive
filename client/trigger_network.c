#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>


#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include "trigger_network.h"

/* Sends a packet starting with the ip header.
 * this means that the IP header is manually specified
 * along whatever else needs to go on the wire.
 *
 * In order to ARP correctly, the IP address should be 
 * passed in so that it can be sent in the sockaddr_in stuct.
 *
 */
void send_RAW_from_IPPROTO(uint32_t addr, uint8_t * wiredata, size_t len)
{
	int s;
	struct sockaddr_in sa;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = addr;
	/* open up a raw socket, need to send the IP header
	 * and whatever else as 'data' with the sendto function
	 */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
#ifdef DEBUG
		perror("Error, cannot create socket");
#endif
		exit(1);
	}

	if (sendto(s, wiredata, len, 0, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
#ifdef DEBUG
		perror("Error, cannot send data");
#endif
		exit(1);
	}

	close(s);
}

int send_UDP_data(uint32_t s_addr, uint32_t d_addr, uint16_t s_port, uint16_t d_port, uint8_t * udp_data, size_t len)
{
	int s;
	struct sockaddr_in source_sa;
	struct sockaddr_in dest_sa;

	source_sa.sin_family = AF_INET;
	source_sa.sin_addr.s_addr = s_addr;
	source_sa.sin_port = s_port;

	dest_sa.sin_family = AF_INET;
	dest_sa.sin_addr.s_addr = d_addr;
	dest_sa.sin_port = d_port;

	/* open up a UDP socket, no spoof except the s_addr may be needed
	 */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {

		perror(" socket(): ");
		DLX(1, printf("Error, cannot create socket\n"));
		close(s);
		return -1;
	}

	if (bind(s, (struct sockaddr *) &source_sa, sizeof(struct sockaddr_in)) == -1) {
		perror(" bind(): ");
		DLX(1, printf("Error, cannot create socket\n"));
		close(s);
		return -1;
	}

	if (sendto(s, udp_data, len, 0, (struct sockaddr *) &dest_sa, sizeof(struct sockaddr_in)) < 0) {
		perror(" bind(): ");
#ifdef DEBUG
		perror("Error, cannot send data");
#endif
		close(s);
		return -1;
	}

	close(s);
	return 0;

}

/* from Computer Networks, 3rd ed. Peterson and Davie, pg 91
 * sz - the number of 16bit words in the buffer
 * buf - the buffer to checksum
 */
uint16_t cksum(uint16_t * buf, uint32_t sz)
{
	uint32_t sum = 0;	//32bit sum

	while (sz--) {
		sum += *buf++;
		if (sum & 0xFFFF0000) {
			sum &= 0xFFFF;
			sum++;
		}
	}
	return ~(sum & 0xFFFF);
}

int send_TCP_data(uint32_t s_addr, uint32_t d_addr, uint16_t s_port, uint16_t d_port, uint8_t * tcp_data, size_t len)
{
	int s;
	struct sockaddr_in source_sa;
	struct sockaddr_in dest_sa;

	source_sa.sin_family = AF_INET;
	source_sa.sin_addr.s_addr = s_addr;
	source_sa.sin_port = s_port;

	dest_sa.sin_family = AF_INET;
	dest_sa.sin_addr.s_addr = d_addr;
	dest_sa.sin_port = d_port;

	/* open up a UDP socket, no spoof except the s_addr may be needed
	 */
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DLX(1, printf("Error, cannot create socket"));
		perror(" socket(): ");
		return -1;
	}

	if (bind(s, (struct sockaddr *) &source_sa, sizeof(struct sockaddr_in)) == -1) {
		DLX(1, printf("Error, cannot bind socket"));
		perror(" bind(): ");
		close(s);
		return -1;
	}

	if (connect(s, (struct sockaddr *) &dest_sa, sizeof(dest_sa)) < 0) {
		DLX(1, printf("Error, cannot connect TCP socket"));
		perror(" connect(): ");
		close(s);
		return -1;

	}

	if (send(s, tcp_data, len, 0) < 0) {

#ifdef DEBUG
		perror("Error, cannot send data");
#endif
		close(s);
		return -1;
	}

	close(s);

	return 0;
}
