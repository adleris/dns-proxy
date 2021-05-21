#include "output.h"


void log_dns_request_packet(struct dns_message dns){
    /* setup */
    FILE *fp;
    fp = fopen(FILENAME, "a");

    char *domain_name = (char*)calloc(MAX_URL_LABELS, sizeof(char));
    for (int u=0; u<dns.question.num_url_labels; u++){
        strcat(domain_name, dns.question.url[u]);
        if (u+1 < dns.question.num_url_labels){
            strcat(domain_name, ".");
        }
    }


    /* request */
    if (dns.header.ancount == 0){
        print_timestamp(fp);
        fprintf(fp, " requested %s\n", domain_name);
    } 

    /* AAAA record? */
    if (dns.question.qtype == TYPE_AAAA){

        /* answer */
        if (dns.header.ancount > 0){
            print_timestamp(fp);
            fprintf(fp, " %s is at %s\n", domain_name, ipv6_print_string(dns.answer.rdata));
        }
    } else { /* not AAAA, unimplemented request */
        print_timestamp(fp);
        fprintf(fp, " unimplemented request\n");
    }

    fflush(fp);
    fclose(fp);
    free(domain_name);
}

void log_dns_response_packet(struct dns_message dns){
    /* setup */
    FILE *fp;
    fp = fopen(FILENAME, "a");

    char *domain_name = (char*)calloc(MAX_URL_LABELS, sizeof(char));
    for (int u=0; u<dns.question.num_url_labels; u++){
        strcat(domain_name, dns.question.url[u]);
        if (u+1 < dns.question.num_url_labels){
            strcat(domain_name, ".");
        }
    }

    /* the response should exist and should be AAAA */
    if (dns.header.ancount >= 1 && dns.answer.type == TYPE_AAAA){
        /* answer: print the first matching record */
        print_timestamp(fp);
        fprintf(fp, " %s is at %s\n", domain_name, ipv6_print_string(dns.answer.rdata));
    }

    fflush(fp);
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
