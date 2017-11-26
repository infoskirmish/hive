#ifndef __TRIGGER_H
#define __TRIGGER_H

#include "trigger_protocols.h"
#include "trigger_utils.h"
#include "debug.h"

struct trigger_params 
{
	int		padding;
	char		*callback_ip;
	char		*callback_port;
	char		*target_ip;
	char		*raw_port;
	char		*type;
	unsigned char	triggerKey[ID_KEY_HASH_SIZE];
};

void * trigger_start( void * );
int parse_trig( const char * str, uint32_t* trig);

#endif
