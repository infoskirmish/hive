#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include "get_data.h"
#include "proj_strings.h"
#include "debug.h"
#include "run_command.h"

unsigned char* get_data(int* size, int flag)
{
	int retVal = 0;
	unsigned char* buf = NULL;
	unsigned char* cmd_str = NULL;
	buf = (unsigned char*) malloc(*size);
	memset(buf,0,*size);

	switch (flag)
	{
	case GD_PROC_LIST:
		cmd_str = bus1;

		break;
	case GD_IPCONFIG:
		cmd_str = bus2;

		break;
	case GD_NETSTAT_AN:
#if SOLARIS
		cmd_str = bb22;
#else
		cmd_str = bb2;
#endif
		break;
	case GD_NETSTAT_RN:
		cmd_str = bb1;
		break;
	}

	retVal = run_command(cmd_str,buf,size);
	if(retVal == 1)
	{
		free(buf);
		buf = (unsigned char*) malloc(*size);
		memset(buf,0,*size);
		retVal = run_command(cmd_str,buf,size);
		if(retVal == -1 || retVal == 1)
		{
			D(printf("Could not get process list!\n"));
			free(buf);
			return NULL;
		}
	}
	else if( retVal == -1)
	{
		D(printf("Could not get process list!\n"));
		free(buf);
		return NULL;
	}

	return buf;
}
