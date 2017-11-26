#ifndef __COMPAT_H
#define __COMPAT_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

// LINUX & SOLARIS include files
#if defined LINUX || defined SOLARIS
#include "function_strings.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <signal.h>

#ifdef LINUX
#include <getopt.h>
#endif

#endif

#define closesocket(x) close(x)
#define sprintf_s(x, ...) snprintf(x, __VA_ARGS__)
#define SOCKET_ERROR -1
#define INVALID_SOCKET ~0
#define USHORT	unsigned short

#define UPTIME_STR_LEN 256
#define DEFAULT_INITIAL_DELAY	3 * 60			// 3 minutes
#define DEFAULT_BEACON_PORT		443				// TCP port 443 (HTTPS)
#define DEFAULT_BEACON_INTERVAL	0				// operators did not want a default beacon interval
#define DEFAULT_TRIGGER_DELAY	60				// 60 seconds
#define DEFAULT_BEACON_JITTER	3				// integer for percentage of variance [0-30] range

#define SELF_DEL_TIMEOUT 60 * 60 * 24 * 60		// 60 secs * 60 mins * 24 hours * 60 days

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef SHUTDOWN
#define SHUTDOWN 2
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

#if defined SOLARIS || defined WIN32
char exe_path[256];
#endif
///////////////////////////////

//#define _CRTDBG_MAP_ALLOC //IAN DELETE LATER IAN COMMENT OR DELETE
//#include <crtdbg.h> //IAN DELETE LATER IAN COMMENT OR DELETE

#endif //__COMPAT_H
