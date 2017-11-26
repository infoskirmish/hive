#ifndef __NETSTAT_RN_COMMAND_H
#define __NETSTAT_RN_COMMAND_H

#include "function_strings.h"

unsigned char* get_netstat_rn(int* size);
void release_netstat_rn(unsigned char* netstat_rn);

//#ifdef _NETSTAT_RN
//int get_netstat_rn(unsigned char* buf, int* size);  //Mikrotik netstat -rn command function call
unsigned char* get_netstat_rn(int* size);
void release_netstat_rn(unsigned char* netstat_rn);
//#endif

#endif
