/*!
 *  @file util.c
 *
 *   This file provides a variety of useful routines to free memory,
 *   remove files, complement character strings, etc.
 *
 */

#include <string.h>
#include <fcntl.h>  // open, O_RDWR, file stuff
#include <sys/types.h>
#include <errno.h>

#include "string_utils.h"

void 
cl_string(unsigned char *str, int len) 
{
    int i;
    for (i = 0; i< len; i++) {
        str[i] = ~str[i];
    }

}
