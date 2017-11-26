#ifndef __NETSTAT_AN_COMMAND_H
#define __NETSTAT_AN_COMMAND_H

#include "function_strings.h"

#if defined LINUX || defined SOLARIS
unsigned char* get_netstat_an(int* size);
void release_netstat_an(unsigned char* netstat_an);
#endif

#ifdef _NETSTAT_AN
//int get_netstat_an(unsigned char* buf, int* size);  //Mikrotik netstat -antu command function call
unsigned char* get_netstat_an(int* size);
void release_netstat_an(unsigned char* netstat_an);
#endif


#endif
