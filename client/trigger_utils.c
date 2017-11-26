#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>

#include "trigger_utils.h"

static int RAND_INIT = 0;

/*
 * Translation Table as described in RFC1113
 */
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Translation Table to decode (created by author)
 */
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";


short randShort(){

  if(!RAND_INIT){
    rand_init();
  }
  
  return (short)rand();
  
}

char randChar(){

  if(!RAND_INIT){
    rand_init();
  }

  return (char)rand();  
}


void rand_init() {

  struct timeval tod;

  if ( RAND_INIT ) return;	// already initialized

  gettimeofday(&tod,NULL);
  srand( (tod.tv_usec ^ tod.tv_sec) ^ getpid() );

  RAND_INIT = 1;
  return;
}

uint16_t 
tiny_crc16(const uint8_t * msg, uint32_t sz){
  
  uint32_t index;
  uint16_t crc;
  uint8_t val, t;

  /* 
   * CRC16 Lookup tables (High and Low Byte) for 4 bits per iteration. 
   */
  unsigned short CRC16_LookupHigh[16] = {
    0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
    0x81, 0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1
  };
  unsigned short CRC16_LookupLow[16] = {
    0x00, 0x21, 0x42, 0x63, 0x84, 0xA5, 0xC6, 0xE7,
    0x08, 0x29, 0x4A, 0x6B, 0x8C, 0xAD, 0xCE, 0xEF
  };
  
  /*
   * CRC16 "Register". This is implemented as two 8bit values
   */
  unsigned char CRC16_High, CRC16_Low;
  // Initialise the CRC to 0xFFFF for the CCITT specification
  CRC16_High = 0xFF;
  CRC16_Low = 0xFF;

  for (index =0; index < sz; index++){

    val = msg[index] >> 4;

    // Step one, extract the Most significant 4 bits of the CRC register
    t = CRC16_High >> 4;

    // XOR in the Message Data into the extracted bits
    t = t ^ val;

    // Shift the CRC Register left 4 bits
    CRC16_High = (CRC16_High << 4) | (CRC16_Low >> 4);
    CRC16_Low = CRC16_Low << 4;
    
    // Do the table lookups and XOR the result into the CRC Tables
    CRC16_High = CRC16_High ^ CRC16_LookupHigh[t];
    CRC16_Low = CRC16_Low ^ CRC16_LookupLow[t];

    val = msg[index] & 0x0F;

    // Step one, extract the Most significant 4 bits of the CRC register
    t = CRC16_High >> 4;

    // XOR in the Message Data into the extracted bits
    t = t ^ val;

    // Shift the CRC Register left 4 bits
    CRC16_High = (CRC16_High << 4) | (CRC16_Low >> 4);
    CRC16_Low = CRC16_Low << 4;

    // Do the table lookups and XOR the result into the CRC Tables
    CRC16_High = CRC16_High ^ CRC16_LookupHigh[t];
    CRC16_Low = CRC16_Low ^ CRC16_LookupLow[t];
  }
  crc = CRC16_High;
  crc = crc << 8;
  crc = crc ^ CRC16_Low;

  return crc;
}


void
cu_b64_encodeblock( unsigned char in[3],
		    unsigned char out[4], int len )
{

    out[0] = cb64[ in[0] >> 2 ];

    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];

    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');

    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');

}

/*
** decodeblock
**
** decode 4 '6-bit' characters into 3 8-bit binary bytes
*/

void
cu_b64_decodeblock( unsigned char in[4],
		    unsigned char out[3] )

{
    out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);

    out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);

    out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

int
cu_b64_encode_message( const uint8_t * message,
		       uint8_t * output,
		       int message_length,
		       int * output_length)
{
  unsigned char in[3], out[4];
  int i, block_length;int message_index =0;
  int output_index =0;

  if(message == NULL || output == NULL || message_length < 0 ) {
    return FAILURE;
  }

  while( message_index < message_length ) {

    // one block
    for( i = 0, block_length =0; i < 3; i++ ) {

      in[i] =  message_index < message_length ? (unsigned char) message[message_index] : EOF;

      if(  message_index < message_length ) {

	message_index++;
	block_length++;

      } else {
	
	in[i] = 0;

      }

    }

    if( block_length ) {

      cu_b64_encodeblock( in, out, block_length  );

      for( i = 0; i < 4; i++ ) {
	
	output[output_index++] = out[i];
	
      }
      
      
    }

  }

  *output_length = output_index;

  return SUCCESS;
}

int
cu_b64_decode_message( const uint8_t * message,
		       uint8_t * output,
		       int message_length,
		       int * output_length)
{
  unsigned char in[3], out[4], v;
  int i, block_length;
  int message_index =0;
  int output_index =0;

  if(message == NULL || output == NULL || message_length <= 0 ) {
    return FAILURE;
  }

  while( message_index != (message_length+1) ) {

    for( i = 0, block_length =0;
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

      cu_b64_decodeblock( in, out );
      
      for( i = 0; i <  block_length - 1; i++ ) {
	
	output[output_index++] = out[i];
	
      }
      
    }
    
  }

  *output_length = output_index;
  return SUCCESS;

}
