#include <stdlib.h>

#include "compat.h"
#include "trigger_b64.h"
#include "debug.h"


static void 
b64_decodeblock( unsigned char in[4], 
		    unsigned char out[3] );

/*
 * Translation Table to decode (created by author)
 */
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*
** decodeblock
**
** decode 4 '6-bit' characters into 3 8-bit binary bytes
*/

static void 
b64_decodeblock( unsigned char in[4], 
		    unsigned char out[3] )

{   

    out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);

    out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);

    out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);

}

int
b64_decode_message( const uint8_t * message, 
		       uint8_t * output, 
		       int message_length,
		       int * output_length)
{
  unsigned char in[4], out[4], v;
  int i, block_length;
  int message_index =0;
  int output_index =0;

  if(message == NULL || output == NULL || message_length <= 0 ) {
    return FAILURE;
  }

  while( message_index != (message_length+1) ) {

    for( i = 0, block_length = 0; 
	 i < 4 &&  message_index != (message_length +1); 
	 i++ ) 
      {
	
	v = 0;
	
	while( message_index != (message_length+1) && v == 0 ) {
      
	  
	  v = message_index < message_length ? (unsigned char) message[message_index] : EOF;

	  message_index++;

	  v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
	  
	  if( v ) {
	    
	    v = (unsigned char) ((v == '$') ? 0 : v - 61);
	    
	  }	  
	}

	// one block
	if( message_index !=  (message_length+1) ) {

	  block_length++;
	  
	  if( v ) {
	    
	    in[ i ] = (unsigned char) (v - 1);
	    
	  }
	  
	} else {
	  
	  in[i] = 0;
	  
	}

      }
  
    if( block_length ) {

      b64_decodeblock( in, out );
      
      for( i = 0; i <  block_length - 1; i++ ) {
	
		output[output_index++] = out[i];
	
      }
      
    }
    
  }

  *output_length = output_index;
  return SUCCESS;

}
