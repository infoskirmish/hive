#include <iostream>

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include "LibraryModuleBase.h"
#include "Primitive.h"
#include "ProcessCmdResponse.h"
#include "ilm-client.h"
#include "Connection.h"
#include "File.h"

extern "C" {
#include "debug.h"
#include "proj_strings.h"
#include "colors.h"
}

extern Connection *myConn;

using namespace InterfaceLibrary;
using namespace InterfaceLibrary::Primitive;

void File::Delete( Primitive::Activation& actvn, ProcessCmdAccumulator&, ProcessCmdResponse& resp )
{
	String			*argPtr = ( String * )( actvn.arguments );
	string			filename = *argPtr;
	struct send_buf sbuf;
	REPLY rbuf;

	memset( &sbuf, 0, sizeof( struct send_buf ) );
	memset( &rbuf, 0, sizeof( REPLY ) );

	strcat( sbuf.path, filename.c_str() );

#ifdef DEBUG
	cout << " filename: " << filename << endl;
//	printf( " filename: %s\n", sbuf.path );
#endif

	resp.type = ProcessCmdResponse::TYPE_Pending;

	if ( myConn->TxCommand( &sbuf, &rbuf, DELETE ) != SUCCESS )
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " * Error: Problem with or lost server connection." ) );
		return;		
	}

	if (rbuf.reply == 0)
	{
		resp.type = ProcessCmdResponse::TYPE_Success;
		// "successful deletion of remote file %s\n"
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " * Successfully deleted file." ) );
//		printf( "%s %s\n", remove3String, sbuf.path );
	}
	else
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " * Error: Server had difficulty deleting file." ) );
	}

	return;
}

void File::Get( Primitive::Activation& actvn, ProcessCmdAccumulator&, ProcessCmdResponse& resp )
{
	String		*argPtr = (String *)(actvn.arguments);
	String		remote_file = *argPtr++;
	String		local_file = *argPtr;
	string		lfile = local_file;
	string		rfile = remote_file;
	int			fd;
	struct send_buf sbuf;
	REPLY rbuf;

	memset( &sbuf, 0, sizeof( struct send_buf ) );
	memset( &rbuf, 0, sizeof( REPLY ) );

	cout << " Remote File: " << remote_file << endl;
	cout << " Local File: " << local_file << endl;

	resp.type = ProcessCmdResponse::TYPE_Pending;

	if ( ( fd = open( lfile.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 00600 ) ) == ERROR )
	{
		if ( ( errno == EEXIST ) && ( lfile.size() < 248 ) ) 
		{
			lfile += ".XXXXXX";

			if ( ( fd = mkstemp( (char *)lfile.c_str() ) ) == ERROR )
			{
				perror( " mkstemp() " );
				resp.type = ProcessCmdResponse::TYPE_Local_Failure;
				return;
			}
		}
		else
		{
			perror( " open() " );
			resp.type = ProcessCmdResponse::TYPE_Local_Failure;
			return;
		}
	}

	// here we have a valid file descriptor for a unique filename

	strcat( sbuf.path, rfile.c_str() );
	printf( " Downloading to local system as: %s\n", lfile.c_str() );

	if ( myConn->TxCommand( &sbuf, &rbuf, DOWNLOAD ) != SUCCESS )
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " * Error: Problem with or lost server connection." ) );
		return;		
	}

	if ( rbuf.reply == 0 )
	{
		if ( ( rbuf.reply = myConn->RecvFile( fd, ntohl( rbuf.padding ) ) ) == 0 ) 
		{
			// successful download of %d bytes from %s to %s\n
			printf( " %s %d %s %s to %s\n", download5String, ntohl( rbuf.padding ), upload8String, rfile.c_str(), lfile.c_str() );
			resp.type = ProcessCmdResponse::TYPE_Success;
		}
		else
		{
			//application/network errors occurred during download\n
//			printf( " %s", download6String );
			resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
			resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " * Error: Application or network errors occurred during download." ) );
			unlink( lfile.c_str() );
		}
	}
	else
	{
		// unsuccessful download due to problems at remote computer\n
//		printf( " %s", download7String);
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " * Error: Unsuccessful download due to problems at remote computer." ) );
		unlink( lfile.c_str() );
	}

	close( fd );
	return;
}

void File::Put( Primitive::Activation& actvn, ProcessCmdAccumulator&, ProcessCmdResponse& resp )
{
	String		*argPtr = (String *)(actvn.arguments);
	String		local_file = *argPtr++;		// source
	String		remote_file = *argPtr;		// destination
	string		lfile = local_file;
	string		rfile = remote_file;
	int			fd;
	struct stat	st;
	struct send_buf sbuf;
	REPLY rbuf;

	memset( &sbuf, 0, sizeof( struct send_buf ) );
	memset( &rbuf, 0, sizeof( REPLY ) );

	cout << " Local File: " << local_file << endl;
	cout << " Remote File: " << remote_file << endl;

	resp.type = ProcessCmdResponse::TYPE_Pending;

	if ( ( fd = open( lfile.c_str(), O_RDONLY ) ) == ERROR )
	{
		perror( " open() " );
		resp.type = ProcessCmdResponse::TYPE_Local_Failure;
		return;
	}

	// how big is the local file?
	if ( stat( lfile.c_str(), &st ) != SUCCESS )
	{
		// stat failed to return zero
		perror( " stat()" );
		close( fd );
		resp.type = ProcessCmdResponse::TYPE_Local_Failure;
		return;
	}

	// populate the send structure with filename and size
	sbuf.size = htonl( st.st_size );
	strcat( sbuf.path, rfile.c_str() );

	if ( myConn->TxCommand( &sbuf, &rbuf, UPLOAD ) != SUCCESS )
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " * Error: Problem with or lost server connection." ) );
		return;		
	}

	if ( rbuf.reply == 0 )
	{
		if ( ( rbuf.reply = myConn->SendFile( fd, st.st_size ) ) == 0 ) 
		{
			//successful upload of %d bytes from %s to %s\n
			printf( " %s %d %s %s to %s\n", upload7String, (int)st.st_size, upload8String, lfile.c_str(), rfile.c_str() );
			resp.type = ProcessCmdResponse::TYPE_Success;
		}
		else
		{
			//application/network errors occurred during upload\n
//			printf( " %s", upload9String );
			resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " * Error: Application or network errors occurred during upload." ) );
			resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		}
	}
	else
	{
		// unsuccessful upload due to problems at remote computer\n
//		printf( " %s", upload10String);
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " * Error: Unsuccessful upload due to problems at remote computer." ) );
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
	}

	close( fd );
	return;
}
