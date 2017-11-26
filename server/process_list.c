#include "process_list.h"
#include "debug.h"
#include "proj_strings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat.h"

//---------------------------------------------------------------------------------------------------
//NOT USED RIGHT NOW
#if defined LINUX && !defined _PS_EF
#include <unistd.h>
#include <stdlib.h>
#include "get_data.h"

unsigned char* get_process_list(int* size)
{
	return get_data(size,GD_PROC_LIST);
}

void release_process_list(unsigned char* list)
{
	if(list != NULL)
	{
		free(list);
	}
}
#endif

#if defined SOLARIS
#include "get_data.h"

unsigned char* get_process_list(int* size)
{
	return get_data(size,GD_PROC_LIST);
}

void release_process_list(unsigned char* list)
{
	if(list != NULL)
	{
		free(list);
	}
}
#endif
//---------------------------------------------------------------------------------------------------
#if defined _PS_EF    //MikroTik
#include <dirent.h>

//MikroTik Helper Functions and variables...
#define maxStringLength 256 

void displayStatus( char *processDir, char *blockData, int showHeader);
//End of MikroTik Helper functions/variables...

//Called in beacon.c to release data...
void release_process_list(unsigned char* list)
{
	if(list != NULL)
	{
		free(list);
	}
}

// Called in Beacon.c
//int get_process_list(unsigned char* buf, int* size)
unsigned char* get_process_list( int* size )
{
   	char *lineout=NULL;     //Will create and expand as necessary;
   	int lineoutLength=0;
   	char *oldOutput=NULL;   //Pointer to memory that will be freed...
	int showHeader = 1;  //First time, diplay header line

	static int defaultIncrementSize= 4096;
	static int sizeIncrementCount= 1;

	char blockData[1024];

	DIR *dp;
	struct dirent *dirp;

	if ((dp = opendir("/proc")) == NULL)
	{
		//printf("Can't open /proc, we are done...");
		*size = 0;
		return NULL;
	}

	while ((dirp = readdir(dp)) != NULL)
	{

		//All processes in /proc directory start with a number so we'll 
		//  just check the first digit of the d_name...
		//  Note assumption that no process ID starts with 0.
		switch(dirp->d_name[0])
		{
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				//displayStatus( dirp->d_name, &blockData[0], showHeader );
				memset( blockData, '\0', sizeof(blockData) );
				displayStatus( dirp->d_name, blockData, showHeader );
				if (showHeader == 1) showHeader = 0;
				
				//Initialize lineout 
				if (!lineout)
				{
                	lineout = (char *) malloc( defaultIncrementSize );
                	DLX(8, printf(" defaultIncrementSize = %d, sizeof(lineout)=%d\n", defaultIncrementSize, defaultIncrementSize));
                	memset( lineout, '\0', defaultIncrementSize );
					//strncat( lineout, blockData, sizeIncrementCount*defaultIncrementSize -1);
					//printf("\n-----------------------------------------------------\n start\n\n %s\n\nend\n-----------------------------------------------\n", lineout);
					//printf("\n\n Lineout first time initialized.\n\n");
				}
				else
				{

					if ( strlen(blockData) + 1 >  sizeIncrementCount*defaultIncrementSize - strlen(lineout) )
                	{

				      if (oldOutput==NULL)
					  {
							oldOutput = lineout;
					      //printf("\n\n oldOutput points to lineout.\n\n");
					  }
                      sizeIncrementCount++;

                      //Expand lineout
                      lineout = (char *) calloc( sizeIncrementCount*defaultIncrementSize, sizeof(char));   //Add size of 4096 characters to existing length...
                      D( printf(" Expanding lineout again to %i and cleaning\n", sizeIncrementCount*defaultIncrementSize); );
                      memset( lineout, '\0', sizeIncrementCount*defaultIncrementSize);
                      if (oldOutput != NULL)
						{
							strncat (lineout, oldOutput, sizeIncrementCount*defaultIncrementSize -1);
							free(oldOutput);
							oldOutput=NULL;
						}
                	}
     			}
                if (strlen(blockData) > 1)
					strncat( lineout, blockData, sizeIncrementCount*defaultIncrementSize - strlen(lineout) - 1);
				break;
			default:
				break;
		}

	}
	closedir(dp);

	lineoutLength = strlen(lineout) + 1;

	//printf("\n\n size = %i, lineoutLength = %i\n\n", *size, lineoutLength);

	if (oldOutput)  free(oldOutput);
	oldOutput = NULL;

    //Better have lineout
	if (lineoutLength > 1)  
	{
		*size = sizeIncrementCount*defaultIncrementSize;
		return (unsigned char *) lineout;
	}
	else
	{
		*size = 0;
		//Get rid of lineout
		if (lineout)  free(lineout);
		return NULL;
    }

}


//Calling function determines whether we should print the header ...
void displayStatus( char *processDir, char *blockData, int showHeader)
{
   char tempFileName[maxStringLength];
   FILE* tempFile;

   ///proc/[number/stat file fields that will be scanned...
   unsigned int pid;
   char command[maxStringLength];
   char state[maxStringLength];
   unsigned int ppid;
   unsigned int pgrp;
   unsigned int session;
   ///

   //D(printf("%s\n", dirp->d_name);)
   //snprintf( tempFileName, 150, "/proc/%s/stat", processDir);
   snprintf( tempFileName, maxStringLength, "/%s/%s/%s", prc1, processDir, prc2);
   //D(printf( "\n\n tempFileName = %s\n\n", tempFileName);)

   tempFile = fopen( tempFileName, "r");
   if (tempFile)
   {

		if (fscanf( tempFile, "%d %s %s %d %d %d", &pid, command, state, &ppid, &pgrp, &session) > 0)
		{
			//printf("\n\n showHeader = %i\n\n", showHeader);
    		if (showHeader != 0)
	   		{
				//printf("\n\n nonzero showHeader = %i\n\n", showHeader);
	       		//snprintf( blockData, 1000, "\npid             state    ppid            pgrp            session        command\n%-15d %-5s    %-15d %-15d %-15d %-100s\n", pid, state, ppid, pgrp, session, command);
	       		snprintf( blockData, 1024, "%s%-5d %-5s %-5d %-5d %-7d %s\n", prc3, pid, state, ppid, pgrp, session, command);
	   		}
			else
			{
			   //printf("\n\n zero showHeader = %i\n\n", showHeader);
	   		snprintf( blockData, 1024, "%-5d %-5s %-5d %-5d %-7d %s\n", pid, state, ppid, pgrp, session, command);
			}
		}
      //printf("tempFileName = %s\n", tempFileName);

      fclose(tempFile);
   }

	return;

}

#endif
