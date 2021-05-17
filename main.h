#ifndef MAIN_H
#define MAIN_H

#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dns.h"

#define QUERY_PORT_NO  8053
#define QUERY_PORT_STR "8053"

#define ACCEPTED_QTYPE TYPE_AAAA


/* create a new socket (calls socket, bind, listen) */
int new_listening_socket(char *address, char *port);

#endif
