#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DNS_HEADER_LENGTH 12 /*bytes*/
#define MAX_URL_LABELS 255   /* given by URL specifications I'm pretty sure */

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

int main(int argc, char* argv[]) {
    /* read in the length of the message and fix byte-ordering */
    uint16_t message_length;
    if (fread(&message_length, sizeof(uint16_t), 1, stdin) != 1) {
        exit(EXIT_FAILURE);
    }
    message_length = ntohs(message_length);
    printf("scan in length %" PRId16 "\n", message_length);

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

    printf("---- Header ----\n");
    printf("id:      0x%04" PRIx16 "\n", header.id);
    printf("flags:   0x%04" PRIx16 "\n", header.flags);
    printf("qdcount: %" PRId16 "\n", header.qdcount);
    printf("ancount: %" PRId16 "\n", header.ancount);
    printf("nscount: %" PRId16 "\n", header.nscount);
    printf("arcount: %" PRId16 "\n", header.arcount);

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

    printf("---- Question ----\n");
    for (int i = 0; i < question.num_url_labels; i++){
        printf("url[%d] = %s\n", i, question.url[i]);
    }

    return 0;
}
