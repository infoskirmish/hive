#include "hclient.h"
#include "debug.h"
#include "colors.h"

#include "proj_strings.h"    //Required for string manipulation....
#include <pthread.h>

extern pthread_mutex_t 		tlock;

void GenRandomBytes( char *b1, int s1, char *b2, int s2 )
{
	int i;

    //srand((unsigned int)rand());  NOT NEEDED...

	// fill the first buffer
	for ( i=0; i < s1; i++ )
	{
		b1[i] = (char)(rand() % 255);
	}

	// fill the second buffer
	if ( b2 != NULL )
	{
		for ( i=0; i < s2; i++ )
		{
			b2[i] = (char)(rand() % 255);
		}
	}

	return;
} 

/* ******************************************************************************************************************************
 *
 * Usage(char* progname)
 * Description -- function outputs the programs options and parameters in case the operator asks for it or when the
 *                operator makes a mistake at the shell prompt
 * Parameters  -- progname = name of the binary executable used to start process
 * Return      -- void
 *
 * **************************************************************************************************************************** */

void Usage(char* progname) {
   //fprintf(stdout, "\n%sUsage:%s\n", BLUE, RESET );
   fprintf(stdout, "\n  %s%s:%s\n", BLUE, UsageString, RESET );

   //fprintf(stdout, "  %s [-p port] \n", progname);
   fprintf(stdout, "  %s %s", progname, usageOption01String);

   //fprintf(stdout, "  %s [-p port] [-t address] [-a address] [-P protocol] [-m mode] \n\n", progname);
   fprintf(stdout, "  %s %s", progname, usageOption02String);

   //fprintf(stdout, "  Depending on options, client can send triggers, listen, or both\n");
   fprintf(stdout, "%s", usageOption03String);

   //fprintf(stdout, "    [-p port]      - callback port\n");
   fprintf(stdout, "%s", usageOption04String);

   //fprintf(stdout, "    -t address   - IP address of target\n");
   fprintf(stdout, "%s", usageOption05String);

   //fprintf(stdout, "    -a address   - IP address of listener\n");
   fprintf(stdout, "%s", usageOption06String);

   //fprintf(stdout, "    -P protocol  - trigger protocol (raw-tcp or raw-udp)\n");
   fprintf(stdout, "%s", usageOption07String);

   //fprintf(stdout, "    [-r raw_port]  - when using raw triggers, this specifies which port to send the trigger\n");
   fprintf(stdout, "%s", usageOption08String);

   //fprintf(stdout, "    [-m mode]      - client to listen-only, trigger-only, or both (default) ('l', 't', 'b')\n");
   fprintf(stdout, "%s", usageOption09String);

   //fprintf(stdout, "    -k ID key     - ID key\n");
   fprintf(stdout, "%s", usageOption10String);

   //fprintf(stdout, "    -k ID key file     - ID key filename\n");
   fprintf(stdout, "%s", usageOption11String);

   //fprintf(stdout, "    [-h ]          - print this usage\n\n");
   fprintf(stdout, "%s", usageOption12String);
#ifdef DEBUG
   fprintf(stdout, "    [-D <debug level>]      - debug level\n");
#endif
}

/* ******************************************************************************************************************************
 *
 * ReadScript(char* filename)
 * Description -- function reads a script file to gather all commands to execute in automatic mode; commands are entered
 *                into a char** vector
 * Parameters  -- filename = filename of the script file
 * Return      -- returns a char** vector on success and NULL on failure
 *
 * **************************************************************************************************************************** */

char** ReadScript(char* filename) {
   unsigned short count = 0;
   unsigned short allocated = 5;
   char cline[525];
   char** argv;
   FILE* fp;

   if ((argv = (char**)malloc(allocated * sizeof(char*))) == NULL) {
      //fprintf(stderr, "ReadScript(): failure to allocate memory for the command vector\n");
      fprintf(stderr, "%s", readScriptString1);
      return NULL;
   }
   argv[count] = '\0';
   if ((fp = fopen(filename, "r")) == NULL) {
      perror("ReadScript()");
      FreeArgv(argv);
      argv = NULL;
   } else {
      while (fgets(cline, 525, fp) != NULL) {
         cline[strlen(cline) - 1] = '\0';
         if (count == (allocated - 1)) {
            allocated += 5;
            if ((argv = (char**)realloc(argv, (allocated * sizeof(char*)))) == NULL) {
               //fprintf(stderr, "ReadScript(): failure to reallocate memory for the command vector\n");
               fprintf(stderr, "%s", readScriptString2);
               fclose(fp);
               FreeArgv(argv);
               argv = NULL;
               break;
            }
         }
         argv[count] = strdup(cline);
         count++;
         argv[count] = '\0';
      }
      fclose(fp);
   }

   return argv;
}

int TcpInit( struct proc_vars* info )
{
	int			tempfd;
	socklen_t	len;
	int			n = 1;

	info->local.sin_family = AF_INET;
	info->local.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( ( info->tcpfd = socket( AF_INET, SOCK_STREAM, 0 ) ) == ERROR )
	{
		//fprintf( stderr, " TcpInit(): failure creating TCP/IP socket\n" );
		fprintf( stderr, "%s", tcpInitString );
		return ERROR;
	}

	if ( info->listen == YES )
	{          
		/* Passive Connections */
		tempfd = info->tcpfd;

		if ( setsockopt( tempfd, SOL_SOCKET, SO_REUSEADDR, (void *)&n, sizeof( n ) ) < 0 )
		{
			perror( " setsockopt()" );
			//fprintf( stderr, " Cannot reuse TCP port.  Use another or wait until this port is released.\n" );
			fprintf( stderr, "%s", reuseTcpPortString );
		}

		if ( bind( tempfd, ( struct sockaddr * )&info->local, sizeof( info->local ) ) == ERROR )
		{
			perror( " TcpInit()" );
			close(tempfd);
			return ERROR;
		}

		if ( listen( tempfd, 1 ) == ERROR )
		{
			perror( " TcpInit()" );
			close( tempfd );
			return ERROR;
		}

		// ready for connect backs...so release the lock and send the trigger
		pthread_mutex_unlock( &tlock );

		// executing this function here in anticipation of the accept() call below
		// and to make sure the pthread_mutex_lock is released and allowing
		// the trigger thread to get it.
		// this is a race condition, but none critical [at this stage] as it only affects the order
		// in which output is printed to screen
		len = sizeof( info->remote );

		pthread_mutex_lock( &tlock );
		//fprintf( stdout, "\n %sListening for a connection on port %d ...%s\n", BLUE, ntohs( info->local.sin_port ), RESET );
		fprintf( stdout, "\n %s%s %d ...%s\n", BLUE, listeningString, ntohs( info->local.sin_port ), RESET );
		pthread_mutex_unlock( &tlock );

		if ( ( info->tcpfd = accept( tempfd, ( struct sockaddr * )&info->remote, &len ) ) == ERROR )
		{
			//fprintf( stderr, " TcpInit(): failure accepting the TCP/IP connection\n" );
			fprintf( stderr, "%s", tcpInitFailString );
			close( tempfd );
			return ERROR;
		}

		close(tempfd);
		
	}

	len = sizeof( info->local );

	if ( getsockname( info->tcpfd, ( struct sockaddr* )&info->local, &len ) == ERROR )
	{
		perror( " TcpInit()" );
	}

	//fprintf( stdout, " %s... connection established%s\n", BLUE, RESET );
	fprintf( stdout, "%s%s%s\n", BLUE, connectionEstString, RESET );

	return 0;
}

/* ******************************************************************************************************************************
 *
 * OpenFile(char* filename, struct proc_vars* info)
 * Description -- opens the file to be uploaded/downloaded from/to the local computer
 * Parameters  -- filename = name of the file on the local computer
 *                info     = pointer to the process data structure
 * Return      -- the file descriptor on success and negative one (-1) on failure
 *
 * **************************************************************************************************************************** */

int OpenFile(char* filename, struct proc_vars* info) {
   int fd;

   if (info->command == UPLOAD) {
      if ((fd = open(filename, O_RDONLY)) == ERROR) {
         perror("\tOpenFile()");
         return ERROR;
      }
   } else {
      if ((fd = open(filename, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 00600)) == ERROR) {
         if ((errno == EEXIST) && (strlen(filename) <= 247)) {
            strncat(filename, ".XXXXXX", 7);
            if ((fd = mkstemp(filename)) == ERROR) {
               perror("\tOpenFile()");
               return ERROR;
            }
         } else {
            perror("\tOpenFile()");
            return ERROR;
         }
      }
   }

   return fd;
}

/* ******************************************************************************************************************************
 *
 * DisplayStatus(struct proc_vars* info)
 * Description -- function displays to the user the status of the current connection and logs the information into the
 *                activity file as well
 * Parameters  -- info = pointer to the process data structure
 * Return      -- void
 *
 * **************************************************************************************************************************** */

void DisplayStatus(struct proc_vars* info) {
   int argc = 0;
   char* message;

//   fprintf(stdout, "\n****************************************************************\n\n");
   //fprintf(stdout, "\n %sSession configuration parameters:%s\n", BLUE, RESET);
   fprintf(stdout, "\n %s%s:%s\n", BLUE, sessionConfigParamString, RESET);

/*
   if (info->listen == NO) {
      fprintf(stdout, "   TCP socket type = connect (active)\n");
   } else {
      fprintf(stdout, "   TCP socket type = listen (passive)\n");
   }
*/

   if (info->interactive == YES) {
      //fprintf(stdout, "  . Interactive mode established\n");
//      fprintf(stdout, "%s", interactiveModeString);
   } else {
      if (info->ignore == NO) {
         //fprintf(stdout, "   Automatic mode established (not ignoring errors)\n");
         fprintf(stdout, "%s", automaticMode1String);
      } else {
         //fprintf(stdout, "   Automatic mode established (ignoring errors)\n");
         fprintf(stdout, "%s", automaticMode2String);
      }
      fprintf(stdout, "%s", message);
      free(message);
      while (info->cstring[argc] != '\0') {
         //(void) asprintf(&message, "      Command string [%d] = %s\n", argc, info->cstring[argc]);
         (void) asprintf(&message, "%s [%d] = %s\n", commandString, argc, info->cstring[argc]);
         fprintf(stdout, "%s", message);
         free(message);
         argc++;
      }
   }
   //(void) asprintf(&message, "  . Remote IP address %s on port %d\n", inet_ntoa(info->remote.sin_addr), ntohs(info->remote.sin_port));
   (void) asprintf(&message, "%s %s %s %d\n", remoteIPAddressString, inet_ntoa(info->remote.sin_addr), onPortString, ntohs(info->remote.sin_port));
   fprintf(stdout, "%s", message);
   free(message);
   //(void) asprintf(&message, "  . Local IP address %s on port %d\n", inet_ntoa(info->local.sin_addr), ntohs(info->local.sin_port));
   (void) asprintf(&message, "%s %s %s %d\n", localIPAddressString, inet_ntoa(info->local.sin_addr), onPortString, ntohs(info->local.sin_port));
   fprintf(stdout, "%s", message);
   free(message);
//   fprintf(stdout, "\n****************************************************************\n\n");
}
