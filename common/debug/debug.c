/*
 * debug.c
 *
 */
#include "debug.h"

void debug_print_buffer(const unsigned char *buf, const size_t len)
{
    size_t i;

	for( i = 0; i < len; i++ ) {

        if( i >= 8192 ) {
        	fprintf(stdout, "\n\tOutput truncated at 8192 bytes...");
            break;
        }

        if (i % 16 == 0)
        	fprintf(stdout, "\n\t%4x: ", (unsigned int)i);
        else {
        	fprintf(stdout, " ");
        	if (i % 8 == 0)
        		fprintf(stdout, " ");
        }

        fprintf (stdout, "%02x", buf[i]);

    }
	fprintf(stdout, "\n");
    if( (len+1) % 16 )
        fprintf(stdout, "\n");
}
