#ifndef OUTPUT_H
#define OUTPUT_H

#include <string.h>
#include <time.h>

#include "dns.h"

void phase1_output(struct dns_message dns);
void print_timestamp(FILE *fp);

#endif 
