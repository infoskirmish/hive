#if defined LINUX || defined SOLARIS
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "unpifi.h"
#endif

#include <features.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include <string.h>
#include <stdlib.h>

#include "trigger_sniff.h"
#include "trigger_listen.h"
#include "compat.h"
#include "debug.h"

//*************************************************************************
/*  This is the only external function in the file,
 *  all other funcs are private static helpers to this.
 */
int dt_get_socket_fd()
{
  int fd;

  fd = dt_create_raw_socket();

  if ( fd == -1 )
  {
    return FAILURE;
  } 

  // TODO: does ETH_P_IP need to be part of the function definition???
//  if ( dt_find_interface_and_bind(fd, ETH_P_IP) != SUCCESS )
  if ( dt_find_interface_and_bind(fd, 0) != SUCCESS )
  {
      return FAILURE;
  }

  return fd;
}

//*************************************************************************
#if defined LINUX
int dt_find_interface_and_bind(int fd, int proto)
{
	// to silence compiler warnings
	fd = fd;
	proto = proto;

	// listen on all interfaces
	return SUCCESS;
}

//************************************************************************
int dt_create_raw_socket()
{
	int raw_fd;
  
	if ( ( raw_fd = socket( PF_PACKET, SOCK_RAW, htons( ETH_P_IP ) ) ) == -1 )
	{
		return FAILURE;
	}

	return raw_fd;
}

#endif  //_#if defined LINUX
