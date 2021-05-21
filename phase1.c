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
    
    size_t length = read_client_request(fd, &buffer);
    parse_request(&dns_request, &buffer, length);
    log_dns_request_packet(dns_request);
}
