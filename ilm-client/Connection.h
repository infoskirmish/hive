#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "ProcessCmdResponse.h"
#include "LibraryModuleBase.h"

#include "crypto.h"
#include "ilm-client.h"

//*****************************************************************************************
enum ConnectionState
{
	CONNERROR = -2,
	NOCONNECTION,
	LISTENING,
	CONNECTED,
};

//*****************************************************************************************
class Connection
{
	public:

	// methods
		Connection();
		virtual ~Connection();
		int Close( void );
		int Listen( uint16_t port );
		int Accept( std::string& ip );
		int TxCommand( struct send_buf* sbuf, REPLY* rbuf, unsigned char command_code );
		int RecvFile( int fd, int size );
		int SendFile( int fd, int size );

	// properties

	private:
		int					state;
		ctr_drbg_context	ctr_drbg;
		crypt_context		*ioc;
		int					listenfd;
		int					acceptfd;
		struct sockaddr_in	local;
		struct sockaddr_in	remote;
};

#endif
