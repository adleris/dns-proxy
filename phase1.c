#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>

#include "dns.h"

#define DNS_HEADER_LENGTH 12 /*bytes*/
#define MAX_URL_LABELS 255   /* given by URL specifications I'm pretty sure */

#define FILENAME "dns_svr.log"

void phase1_output(struct dns_message);
void print_timestamp(FILE *fp);

int main(int argc, char* argv[]) {
    /* read in the length of the message and fix byte-ordering */
    uint16_t message_length;
    int total_message_offset = 0;
    if (fread(&message_length, sizeof(uint16_t), 1, stdin) != 1) {
        exit(EXIT_FAILURE);
    }
    message_length = ntohs(message_length);

#if DNS_VERBOSE
    printf("packet length %" PRId16 "\n", message_length);
#endif

    /* next, we want to malloc enough space to hold the entire DNS message */
    uint8_t *message = calloc(message_length, sizeof(uint8_t));
    if (message == NULL) {exit(EXIT_FAILURE);}

    if (fread(message, message_length, 1, stdin) != 1){
        exit(EXIT_FAILURE);
    }

    struct dns_header header;
    header.id      = ntohs(* (uint16_t*)message);
    header.flags   = ntohs(* (uint16_t*)(message + 2));
    header.qdcount = ntohs(* (uint16_t*)(message + 4));
    header.ancount = ntohs(* (uint16_t*)(message + 6));
    header.nscount = ntohs(* (uint16_t*)(message + 8));
    header.arcount = ntohs(* (uint16_t*)(message + 10));

    print_dns_header(header);

    total_message_offset += DNS_HEADER_LENGTH;
    message += DNS_HEADER_LENGTH;

    struct dns_question question;
    /* read in the url components */
    question.url = calloc(MAX_URL_LABELS, sizeof(char*));
    question.num_url_labels = 0;


    int offset = 0;
    uint8_t next_length;

    #define LENGTH_NUMBER_BYTES 1 /* the number of bytes that the value storing next_length takes up */
    for (int url_label = 0; (next_length = (* (uint8_t*)(message + offset)) ) != 0; url_label++){

        /* get mem for this url label */
        question.url[url_label] = calloc(next_length + 1, sizeof(char));

        for (int i = 0; i<next_length; i++){
            question.url[url_label][i] = (char)message[offset + i + LENGTH_NUMBER_BYTES];
        }
        question.url[0][next_length] = '\0'; /* shouldn't need to do this if we calloc, but safer */

        offset += next_length + LENGTH_NUMBER_BYTES; // plus ONE is length of the uintEIGHT_t number, and there's +next_length for the cars read
        question.num_url_labels = url_label + 1;
    }

    question.qtype  = ntohs(*(uint16_t*)(message+offset+1));
    question.qclass = ntohs(*(uint16_t*)(message+offset+3));

    total_message_offset += offset + 5;
    message += offset+5;    // previous offset, plus 2 octets from qclass

    print_dns_question(question);


    struct dns_answer answer = {0};
    /* ANSWER SECTION */
    if (header.ancount > 0) {
        answer.name  = ntohs(*(uint16_t*)message);
        answer.type  = ntohs(*(uint16_t*)(message+2));
        answer.class = ntohs(*(uint16_t*)(message+4));
        answer.ttl   = ntohl(*(uint32_t*)(message+6));
        answer.rdlength = ntohs(*(uint16_t*)(message+10));
        answer.rdata = (uint16_t*)calloc(answer.rdlength,sizeof(uint16_t));

        int i;
        for (i = 0; i < answer.rdlength; i += 2){
            answer.rdata[i] = ntohs(*(uint16_t*)(message+12 + i));
        }

        print_dns_answer(answer);

        total_message_offset += 12 + i;
        message += 12 + i;
    } else {
        answer.rdlength = 0; // make sure it's 0 // THIS IS DUMB, POTENTIALLY? NO IDEA. use pointers
#if DNS_VERBOSE
        printf("---- (No Answer) ----\n");
#endif
    }

#if DNS_VERBOSE
    int left = (int)message_length - total_message_offset;
    printf("---- Additional Record ----\n");
    for (int k=0;k<left;k+=2){   /* k is in octets; +=2 means 2 octet sections */
        printf("0x%04"PRIx16"\t", ntohs(*(uint16_t*)(message+k)));
    }
    printf("\n");
#endif

    struct dns_message dns_message;
    dns_message.header = header;
    dns_message.question = question;
    dns_message.answer = answer;
    // dns_message.additional 
    /* task output */
    phase1_output(dns_message);
    return 0;
}

void phase1_output(struct dns_message dns){
    
    FILE *fp;
    fp = fopen(FILENAME, "w");

    char *domain_name = (char*)calloc(255,sizeof(char));
    /* request */
    for (int u=0; u<dns.question.num_url_labels; u++){
        strcat(domain_name, dns.question.url[u]);
        if (u+1 < dns.question.num_url_labels){
            strcat(domain_name, ".");
        }
    }
    print_timestamp(fp);
    fprintf(fp, " requested %s\n", domain_name);

    if (dns.question.qtype == TYPE_AAAA){

        /* check if we have an answer and print that */
        if (dns.answer.rdlength != 0){
            print_timestamp(fp);
            fprintf(fp, " %s is at %s\n", domain_name, ipv6_print_string(dns.answer.rdata));
        }
    } else { /* not AAAA */
        print_timestamp(fp);
        fprintf(fp, " unimplemented request\n");
    }
    
    fclose(fp);
    free(domain_name);
}

/* timestamp */
void print_timestamp(FILE *fp){
    char *timestr = (char*)calloc(255, sizeof(char));
    time_t now;
    struct tm *info;
    time(&now);
    info = localtime(&now);
    strftime(timestr, 255, "%FT%T%z", info);
    fprintf(fp, "%s", timestr);
    free(timestr);
}
