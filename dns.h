#ifndef DNS_H
#define DNS_H 

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/* DISPLAY SETTINGS */
#define DNS_USE_COLOUR 0
#define DNS_VERBOSE    0

#if DNS_USE_COLOUR
#define SET_COLOUR(c) printf("\033[%dm",c)
#define RST_COLOUR() printf("\033[0m")
#define COLOUR() SET_COLOUR(31)
#else
#define SET_COLOUR(c) 
#define RST_COLOUR() 
#define COLOUR()
#endif /* USE_COLOUR */


/* CONSTANTS */
#define TYPE_A      1
#define TYPE_AAAA   28
#define CLASS_IN    1


struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

struct dns_question {
    char     **url;
    int        num_url_labels;
    uint16_t   qtype;
    uint16_t   qclass;
};

struct dns_answer {
    uint16_t  name;
    uint16_t  type;
    uint16_t  class;
    uint32_t  ttl;
    uint16_t  rdlength; /* length is in OCTETS, not UINT16 */
    uint16_t *rdata;    /* length given by rdlength IPv4: 4B, IPv6: 8B*/
};

struct dns_message {
    struct dns_header   header;
    struct dns_question question;
    struct dns_answer   answer;
    struct dns_answer   additional;
};

void print_dns_header(struct dns_header header);
void print_dns_question(struct dns_question question);
void print_dns_answer(struct dns_answer answer);

char *ipv6_print_string(uint16_t *addr);

#endif