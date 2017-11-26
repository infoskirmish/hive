#ifndef _dt_b64_h
#define _dt_b64_h

#include "debug.h"

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

extern int
b64_decode_message( const uint8_t * message, 
		    uint8_t * output, 
		    int message_length,
		    int * output_length);

extern int
b64_encode_message( const uint8_t * message, 
		    uint8_t * output, 
		    int message_length,
		    int * output_length);



#endif
