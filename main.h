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

#define RESPONSE_BUFFLEN 1024

/********************* client connection *********************/
/* create a new socket (calls socket, bind, listen) */
int new_listening_socket(char *address, char *port);

/* accept a client socket */
int accept_client_connection(int sockfd);


/******************** upstream connection *********************/
/* create a connection to the upstream: send and receive data */
size_t dns_upstream_connection(char *address, char *port, uint8_t **response, uint8_t *request, size_t request_len);

/* establish connection to upstream */
int connect_to_upstream(char *address, char *port);

#endif
