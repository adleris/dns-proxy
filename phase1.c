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
    int fd = STDIN_FILENO;
    parse_request(fd);
}
