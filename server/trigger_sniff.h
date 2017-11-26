#ifndef _DT_SNIFF_H
#define _DT_SNIFF_H

#include "function_strings.h"

int dt_get_socket_fd();

int dt_find_interface_and_bind(int fd, int proto);

int dt_create_raw_socket();

int dt_bind_raw_socket( char* device, int raw_fd, int proto);

struct ifi_info * dt_get_ifi_info();

void dt_free_ifi_info( struct ifi_info *ifihead );

int sniff_start_solaris( char *devname );

int sniff_read_solaris( int fd, void *outbuf, int len );

#endif // _DT_SNIFF
