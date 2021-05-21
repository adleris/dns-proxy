#ifndef OUTPUT_H
#define OUTPUT_H

#include <string.h>
#include <time.h>

#include "dns.h"

void log_dns_request_packet(struct dns_message dns);
void log_dns_response_packet(struct dns_message dns);
void print_timestamp(FILE *fp);

#endif 
