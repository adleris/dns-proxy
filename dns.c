#include "dns.h"


/* phase 1 */
size_t read_client_request(int fd, uint8_t **full_message){
    /* read in the length of the message and fix byte-ordering */
    uint16_t message_length;
    if (read(fd, &message_length, 1 * sizeof(uint16_t)) != 1 * sizeof(uint16_t)){
        exit(EXIT_FAILURE);
    }
    uint16_t raw_message_length = message_length;
    message_length = ntohs(message_length);

#if DNS_VERBOSE
    printf("packet length %" PRId16 "\n", message_length);
#endif

    /* next, we want to malloc enough space to hold the entire DNS message */
    *full_message = calloc(message_length + 1 + TWO_BYTE_HEADER, sizeof(uint8_t)); /* stick a null byte at the end, plus message length */
    if (*full_message == NULL) {
        exit(EXIT_FAILURE);
    }
    /* store message length at start of buffer */
    (*(uint16_t**)full_message)[0] = raw_message_length;

    /* read, maybe over multiple packets */
    int this_read_len = 0, total_read_len=0;
    while(1){
        this_read_len = read(fd, *full_message + TWO_BYTE_HEADER + total_read_len, (message_length-total_read_len) * sizeof(**full_message));
        total_read_len += this_read_len;
        if (total_read_len >= message_length){
            break;
        }
    }

    return message_length;
}



size_t parse_request(struct dns_message *dns_request, uint8_t **request_buffer, size_t message_length) {

    /* read in the length of the message and fix byte-ordering */
    uint8_t *message = *request_buffer+TWO_BYTE_HEADER;
    int total_message_offset = 0;

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
        question.url[url_label][next_length] = '\0'; /* shouldn't need to do this if we calloc, but safer */

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
    COLOUR();
        printf("---- (No Answer) ----\n");
    RST_COLOUR();
#endif
    }

#if DNS_VERBOSE
    COLOUR();
    int left = (int)message_length - total_message_offset;
    printf("---- Additional Record ----\n");
    for (int k=0;k<left;k+=2){   /* k is in octets; +=2 means 2 octet sections */
        printf("0x%04"PRIx16"\t", ntohs(*(uint16_t*)(message+k)));
    }
    printf("\n");
    RST_COLOUR();
#endif

    /* set outputs */
    dns_request->header = header;
    dns_request->question = question;
    dns_request->answer = answer;
    // dns_request->additional 
    return message_length + TWO_BYTE_HEADER;
}


bool is_AAAA_record(struct dns_message dns_request){
    if (dns_request.question.qtype == TYPE_AAAA) {
        return true;
    }
    return false;
}

/* handle the flags in the header */
void set_rcode_dns(struct dns_message *dns_request, uint16_t code){
    dns_request->header.flags |= code;
}

void set_rcode_char(uint8_t *request_buffer, uint16_t code){
    ((uint16_t*)request_buffer)[RCODE_POS_IN_BUFFER] |= code;
}

bool is_rcode_set(struct dns_message *dns_request, uint16_t code){
    return (bool) dns_request->header.flags & code;
}



/* verbose output ---------------------------------------------------------- */

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
