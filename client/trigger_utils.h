#ifndef _COMMON_UTILS_H
#define _COMMON_UTILS_H
/*!
	\file	common_utils.h
	\brief	General function typically reusable across projects
	
	This file provides functionality to parse POSIX command line options.
	There are only two options, --help and --file.  Help prints a usage
	message, while --file, which is optional, specifies the path to the firmware
	image file that is to be parsed.  Note that if --file is not specified then
	the program considers the second argument to be the file name.
	
*/

#include <stdint.h>

/* struct _cu_stack_node { */
/*   void * data; */
/*   struct _cu_stack_node * prev; */
/* }; */
/* typedef struct _cu_stack_node cu_stack_node; */

/* typedef struct { */
/*   cu_stack_node * top; */
/*   cu_stack_node * bottom; */
/*   int size; */
/* } cu_stack; */

/* extern void* cu_stack_pop(cu_stack * stack); */
/* extern void* cu_stack_top(cu_stack * stack); */
/* extern int cu_stack_push(cu_stack * stack, void * data); */
/* extern int cu_stack_init(cu_stack ** stack); */
/* extern int cu_stack_destroy(cu_stack * stack); */

/*!
	\brief	Opens a file and reads it into a memory buffer.  
	
	This function opens a file and reads it into a memory buffer.  It
	will allocate(malloc) a memory buffer equal to the file_size of the 
	input_file_name and set the output_memory_ptr to the resulting pointer. 
	The bytes_read value is set to the size of the buffer allocated upon successful
	completion. This function is meant to be used when it is desirable to read in the
	contents of a file into memory immediately and for use exclusively by the allocating
	process.  For larger files that may need to be paged in as needed or shared across
	processes, it is better to use mmap or something similar.
	
	\param input_file_name     The name of the file to read.
	\param output_memory_ptr   A pointer to the memory buffer.
	\param bytes_read          The size of the memory buffer.
	
	\return		The success of the call.
	\retval         SUCCESS (0) success
	\retval         FAILURE (-1) failure
*/
/* extern int  */
/* read_file_bytes (char   *input_file_name, */
/* 		 uint8_t  **output_memory_ptr, */
/* 		 size_t  *bytes_read); */

/*!
	\brief	Returns a CRC16-CCITT calculated over the message parameter. 
	
	This function is a self contained CRC16 algorithm. The pre-calculated CRC tables
        are stored as local variables, and the computation is broken down by upper and lower bytes. 
	This function does not allocate any memory and returns the the computed CRC16 in a unsigned
	16bit int. This method has the advantage of being smaller than many CRC16 implementations and 
	does not use global pre-calculated tables, as most implementations do.
	
	\param messsage            A pointer to the message the CRC should be calculated on.
	\param size                The number of bytes (uint8_t) in the message.    
	
	\return		The CRC16 value
	\retval         uint16_t type CRC
*/
extern uint16_t  
tiny_crc16( const uint8_t * msg, 
	    uint32_t sz);

/* extern int randInt(); */

extern short randShort();

extern char randChar();

extern void rand_init();

/* extern void rand_n_char(char * dest, int n) ; */

/* extern int cu_ascii_hex_to_internal(char * ascii_hex, char * output_buffer, int* len); */

// base64 functions
extern int
cu_b64_decode_message( const uint8_t * message,
		       uint8_t * output,
		       int message_length,
		       int* output_length);

extern int
cu_b64_encode_message( const uint8_t * message,
		       uint8_t * output,
		       int message_length,
		       int* output_length);


#ifndef SUCCESS
#define SUCCESS 0
#endif /* SUCCESS */

#ifndef FAILURE
#define FAILURE -1
#endif /* FAILURE */

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#endif /* _COMMON_UTILS_H */
