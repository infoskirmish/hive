#ifndef __PROCESS_LIST_H
#define __PROCESS_LIST_H

#include "function_strings.h"

#if defined LINUX || defined SOLARIS
unsigned char* get_process_list(int* size);
void release_process_list(unsigned char* list);
#endif

#ifdef _PS_EF
//int get_process_list(unsigned char* buf, int *size);
unsigned char* get_process_list(int* size);
void release_process_list(unsigned char* list);
#endif

#endif
