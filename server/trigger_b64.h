#ifndef _dt_b64_h
#define _dt_b64_h

#include "trigger_listen.h"

extern int
b64_decode_message( const uint8_t * message, 
		       uint8_t * output, 
		       int message_length,
		       int * output_length);

#endif
