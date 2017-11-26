#ifndef __SELF_DELETE_H
#define __SELF_DELETE_H

#include "function_strings.h"
#define markTermination jsdfhlwjfgi1k

void self_delete();
void check_timer(char* filepath, unsigned long delete_delay);
void update_file(char* filepath);

#define	SD_PATH_LENGTH		128		// Self delete path length
extern char	sdcfp[SD_PATH_LENGTH];
extern char	sdlfp[SD_PATH_LENGTH];

#if defined LINUX || SOLARIS
void markTermination(char* filepath);	//Not in windows
#endif

#endif

