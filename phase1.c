#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dns.h"

int main(int argc, char *argv[]){
    struct dns_message dns_request = {0};
    int fd = STDIN_FILENO;
    uint8_t *buffer = NULL;
    parse_request(fd, &dns_request, &buffer);
}
