#include <stdio.h> 
#include <stdlib.h>

/*!
 *  @brief cl_string
 *
 *  Takes complement of length (len) character bits within a 
 *  character string (str).
 *
 *  @param str-  Character string that will be complemented.
 *  @param len-  length of characters to be complemented.
 */ 
void cl_string(unsigned char *str, int len);


// makes test cases easier
#define STRING_TEST(x, y) do { if (y == NULL) {fail("value was null");} \
                               else {fail_if ( strncmp(x, y, strlen(x)) != 0, "%s != %s\n", x,y ); } } while (0);

