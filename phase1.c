#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "dns.h"

#define DNS_HEADER_LENGTH 12 /*bytes*/
#define MAX_URL_LABELS 255   /* given by URL specifications I'm pretty sure */

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


    /* ANSWER SECTION */
    if (header.ancount > 0) {
        struct dns_answer answer;
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
#if DNS_VERBOSE
        printf("---- (No Answer) ----\n");
#endif
    }

    int left = (int)message_length - total_message_offset;
#if DNS_VERBOSE
    printf("---- Additional Record ----\n");
    for (int k=0;k<left;k+=2){   /* k is in octets; +=2 means 2 octet sections */
        printf("0x%04"PRIx16"\t", ntohs(*(uint16_t*)(message+k)));
    }
    printf("\n");
#endif

    return 0;
}
