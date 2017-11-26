#include "debug.h"
#include "netstat_rn.h"
#include "proj_strings.h"

#if defined LINUX && !defined _NETSTAT_RN
#include <unistd.h>
#include <stdlib.h>
#include "get_data.h"

unsigned char* get_netstat_rn(int* size)
{
	return get_data(size, GD_NETSTAT_RN);
}

void release_netstat_rn(unsigned char* netstat_rn)
{
	if( netstat_rn != NULL)
	{
		free(netstat_rn);
	}
}

#endif

#if defined SOLARIS
#include "get_data.h"
#include <stdlib.h>

unsigned char* get_netstat_rn(int* size)
{
	return get_data(size, GD_NETSTAT_RN);
}

void release_netstat_rn(unsigned char* netstat_rn)
{
	if( netstat_rn != NULL)
	{
		free(netstat_rn);
	}
}

#endif

#if defined _NETSTAT_RN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "get_data.h"
#include <linux/route.h>

// Called in Beacon.c.   //Mikrotik netstat -rn command function call
//int get_netstat_rn(unsigned char* buf, int* size)
unsigned char* get_netstat_rn(int* size)
{

	char *lineout=NULL;    	//Will create and expand as necessary;
	int lineoutLength=0;
	char *oldOutput=NULL;   //Pointer to memory that will be freed...
    char tempOutput[1024];

    int defaultIncrementSize = 4096;     //New size for incrementing sizes
	int sizeIncrementCount = 1;
	*size = 0;                           //Initialize returned value pointed to by size so it is 0;	
	char fileName[256];

	FILE *fp;
	char line[1024];
	char destinationBuffer[256];
	char gatewayBuffer[256];
	char maskBuffer[256];
						
	//Fields of /proc/net/route
	char interface[25];
	in_addr_t destination;
	in_addr_t gateway;
	unsigned short flags;
	char refCnt[INET_ADDRSTRLEN];
	char use[INET_ADDRSTRLEN];
	char metric[INET_ADDRSTRLEN];
	in_addr_t mask;
	char mtu[INET_ADDRSTRLEN];
	char window[INET_ADDRSTRLEN];
	char irtt[INET_ADDRSTRLEN];
	char* flagBuffer;
	int flagBufferSize = 25;


	//if((fp = fopen("/proc/net/route", "r")) == NULL)
	memset( fileName, '\0', sizeof(fileName) );
	snprintf(fileName, sizeof(fileName)-1, "%s", ntsrn1);
	if((fp = fopen(fileName, "r")) == NULL)
	{
		D(printf("\n\n Can not open file.\n"));
		return NULL;
	}

	//Read the first line which we will ignore, contains field headers...
	if ((fgets(line, 1024, fp)) == NULL) 
	{
		D(printf("\n\n Could not read first line which we ignore...\n"));
		return NULL;	
	}
  
	//fprintf(stdout, "\n%s", line);  //line includes "\n" at the end...
	//fprintf(stdout, " Destination     Gateway         Genmask         Flags   MSS Window  irtt Iface\n");
	//Initialize lineout 
	lineout = (char *) malloc( defaultIncrementSize );
	memset( lineout, '\0', defaultIncrementSize );
	//snprintf( lineout, *size, "\n Destination     Gateway         Genmask         Flags   MSS Window  irtt Iface\n");
	snprintf( lineout, defaultIncrementSize-1, "%s", ntsrn2);
	lineoutLength = strlen(lineout) + 1;

	while ((fscanf(fp, "%s %x %x %hu %s %s %s %x %s %s %s", interface, &destination, &gateway, &flags, refCnt, use, metric, &mask, mtu, window, irtt)) > 0)
	{

		//Convert destination to an ip address and print it...
		inet_ntop(AF_INET, (struct in_addr *) &destination, destinationBuffer, sizeof(destinationBuffer));

		//Convert gateway to an ip address and print it...
		inet_ntop(AF_INET, (struct in_addr *) &gateway, gatewayBuffer, sizeof(gatewayBuffer));

		//Convert mask to a netmask format and print it out...
		inet_ntop(AF_INET, (struct in_addr *) &mask, maskBuffer, sizeof(maskBuffer));

		//Create and calculate flagBuf
		flagBuffer = (char *) calloc( flagBufferSize, sizeof(char));
		memset( flagBuffer, '\0', flagBufferSize );	
		if (flags & RTF_UP) 
		{
			//	strncat(flagBuffer, "U", sizeof(flagBuffer));
			strncat ( flagBuffer, "U", flagBufferSize - strlen(flagBuffer) -1); 
		}
		if (flags & RTF_GATEWAY)
		{
			// strncat(flagBuffer, "G", sizeof(flagBuffer));
			strncat ( flagBuffer, "G", flagBufferSize - strlen(flagBuffer) -1); 
		}
		if (flags & RTF_HOST)
		{
			// strncat(flagBuffer, "H", sizeof(flagBuffer));
			strncat ( flagBuffer, "H", flagBufferSize - strlen(flagBuffer) -1); 
		}
		if (flags & RTF_REINSTATE) 
		{
			// strncat(flagBuffer, "R", sizeof(flagBuffer));
			strncat ( flagBuffer, "R", flagBufferSize - strlen(flagBuffer) -1); 
		}
		if (flags & RTF_DYNAMIC) 
		{
			// strncat(flagBuffer, "D", sizeof(flagBuffer));
			strncat ( flagBuffer, "D", flagBufferSize - strlen(flagBuffer) -1); 
		}
		if (flags & RTF_MODIFIED) 
		{
			// strncat(flagBuffer, "M", sizeof(flagBuffer));
			strncat ( flagBuffer, "M", flagBufferSize - strlen(flagBuffer) -1); 
		}
		//D( printf(" \n\n\n FlagBuffer = [%s] \n\n\n", flagBuffer); );

		//print out routing information...	
		//fprintf(stdout, "%-15s %-15s %-15s %-7s %3s %-7s %4s %-25s\n", destinationBuffer, gatewayBuffer, maskBuffer, flagBuffer, mtu, window, irtt, interface);
        memset( tempOutput, '\0', sizeof(tempOutput));
		snprintf( tempOutput, sizeof(tempOutput)-1, " %-15s %-15s %-15s %-7s %3s %-7s %4s %-25s\n", destinationBuffer, gatewayBuffer, maskBuffer, flagBuffer, mtu, window, irtt, interface);
		//D( printf( "tempOutput = %s", tempOutput); );
 
		if ( strlen(tempOutput) + 1 >  sizeIncrementCount*defaultIncrementSize - strlen(lineout) )
		{ 
			if (oldOutput==NULL) 
			{
				oldOutput = lineout;
			}
	        sizeIncrementCount++;
	
			//Expand lineout
			lineout = (char *) calloc( sizeIncrementCount*defaultIncrementSize, sizeof(char));   //Add size of defaultIncrementSize characters to existing length...
			//D( printf(" Expanding lineout again to %i and cleaning\n", sizeIncrementCount*defaultIncrementSize); );
			memset( lineout, '\0', sizeIncrementCount*defaultIncrementSize);
			if (oldOutput != NULL)
			{
				strncat (lineout, oldOutput, sizeIncrementCount*defaultIncrementSize -1);
				if (oldOutput)  free(oldOutput);
				oldOutput = NULL;
			}
		}
		if (strlen(tempOutput) > 1)
			strncat( lineout, tempOutput, sizeIncrementCount*defaultIncrementSize - strlen(lineout) - 1);

		lineoutLength = strlen(lineout) + 1;
	    //D( printf("\n\nlineout contains string[%s].\n\n", lineout); );
		
		//Clean up flagBuffer and oldOutput		
		if (flagBuffer != NULL) free(flagBuffer);
		if (oldOutput != NULL)  free(oldOutput);
		oldOutput = NULL;

	}

	//clean up old items...
	if (oldOutput)  free(oldOutput);
	fclose( fp );
    
	//Recalculate the size of all strings contained in lineout
	lineoutLength = strlen(lineout) + 1;
	//D( printf("\n\n\n\n\n\n   FINAL lineout of length %i contains string[%s].\n\n", lineoutLength, lineout); );

	//Better have lineout
	if (lineoutLength > 1)  
	{
		*size = sizeIncrementCount * defaultIncrementSize;
		return (unsigned char *) lineout;

	}
	else
	{
		*size = 0;

        //Get rid of lineout
		if (lineout != NULL) free(lineout);

		return NULL;
	}

}

void release_netstat_rn(unsigned char* netstat_rn)
{
	if( netstat_rn != NULL)
	{
		free(netstat_rn);
	}
}

#endif

