#include "dns.h"

#if DNS_VERBOSE
void print_dns_header(struct dns_header header){
    COLOUR();
    printf("---- Header ----\n");
    printf("id       0x%04" PRIx16 "\n", header.id);
    printf("flags    0x%04" PRIx16 "\n", header.flags);
    printf("qdcount  %" PRId16 "\n", header.qdcount);
    printf("ancount  %" PRId16 "\n", header.ancount);
    printf("nscount  %" PRId16 "\n", header.nscount);
    printf("arcount  %" PRId16 "\n", header.arcount);
    RST_COLOUR();
}

void print_dns_question(struct dns_question question){
    COLOUR();
    printf("---- Question ----\n");
    for (int i = 0; i < question.num_url_labels; i++){
        printf("url[%d] = %s\n", i, question.url[i]);
    }
    printf("qtype    %d (%s)\n", question.qtype,  (question.qtype == TYPE_A)?"A":((question.qtype == TYPE_AAAA)?"AAAA":"other type"));
    printf("qclass   %d%s\n", question.qclass, (question.qclass == CLASS_IN)?" (IN)":"");
    RST_COLOUR();
}

void print_dns_answer(struct dns_answer answer){
    COLOUR();
    printf("---- Answer ----\n");
    printf("name     0x%04"PRIx16"\n", answer.name);
    printf("type     0x%04"PRIx16" (%s)\n", answer.type,  (answer.type == TYPE_A)?"A":((answer.type == TYPE_AAAA)?"AAAA":"other type"));
    printf("class    0x%04"PRIx16"%s\n", answer.class, (answer.class == CLASS_IN)?" (IN)":"");
    printf("ttl      0x%08"PRIx32" (d%"PRId32")\n", answer.ttl, answer.ttl);
    printf("rdlength 0x%04"PRIx16" (d%"PRId32")\n", answer.rdlength, answer.rdlength);
    printf("rdata    ");
    for (int j = 0; j < answer.rdlength; j += 2){
        printf("%04"PRIx16"%c",answer.rdata[j], (j+2 >= answer.rdlength)?'\n':':');
    }
    /* print out ip address representation */
    if (answer.rdlength == 16 /* ipv6 length in octets */) {
        char *ipv6_string = ipv6_print_string(answer.rdata);
        printf("         (%s)\n", ipv6_string);
        free(ipv6_string);
    } /* else skip */
    RST_COLOUR();
}
#else /* ! DNS_VERBOSE */
void print_dns_header(struct dns_header header){}
void print_dns_question(struct dns_question question){}
void print_dns_answer(struct dns_answer answer){}
#endif

char *
ipv6_print_string(uint16_t *addr){
    uint16_t buf[sizeof(struct in6_addr)];

    for (int i=0; i < 8; i++){
        buf[i] = ntohs(addr[i*2]); /* why do we have to call ntohs here? */
    }

    char *print = (char*)calloc(INET6_ADDRSTRLEN, sizeof(char));
    inet_ntop(AF_INET6, buf, print, INET6_ADDRSTRLEN);
    return print;
}
