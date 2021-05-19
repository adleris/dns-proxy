#include "main.h"
#include "dns.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int dns_upstream_connection(char *address, char *port, char *response, uint8_t *request, size_t request_len);

int main(int argc, char* argv[]) {
    
    int sockfd, newsockfd, n;

	struct sockaddr_storage client_addr;
	socklen_t client_addr_size;

	/* set up answer and response loop */
	do {
		sockfd = new_listening_socket(NULL, QUERY_PORT_STR);

		// Accept a connection - blocks until a connection is ready to be accepted
		// Get back a new file descriptor to communicate on
		client_addr_size = sizeof client_addr;
		newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
		if (newsockfd < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		struct dns_message dns_request = {0};
		uint8_t  *request_buffer = NULL;
		size_t dns_request_len = parse_request(newsockfd, &dns_request, &request_buffer);
		char *response_buffer = NULL;
		int response_len;

		if (is_AAAA_record(dns_request) == false){
			set_rcode(&dns_request, RCODE_ERROR);
			/* return the modified request back to sender as response */
			// response_buffer = request_buffer;
			// response_len = dns_request_len;
			response_buffer = NULL;
			response_len = 0;
		} else {
			/* AAAA request is made, forward along to the upstream server */
			// struct dns_message dns_response = {0};
			// int response_len = dns_upstream_connection(argv[1], argv[2], &dns_response, &dns_request, dns_request_len);
			response_len = dns_upstream_connection(argv[1], argv[2], response_buffer, request_buffer, dns_request_len);
		}

		/* return the packet back to the client over that original connection */
		// write(STDIN_FILENO, "responding to our client request\n\t<<", strlen("responding to our client request\n\t<<"));
		// write(STDIN_FILENO, response_buffer, response_len);
		// write(STDIN_FILENO, ">>\n\t^that was our response buffer", strlen(">>\n^that was our response buffer"));
		// printf("\t response buffer is at pointer @%p\n", response_buffer);
		n = write(newsockfd, response_buffer, response_len);
		if (n < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}

		close(sockfd);
		close(newsockfd);
	} while (1);

    return 0;
}

int new_listening_socket(char *address, char *port){
    int sockfd, re, s;
	struct addrinfo hints, *res;

    // Create address we're going to use
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept
	// node (NULL means any interface), service (port), hints, res
	s = getaddrinfo(NULL, port, &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}


	// Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	// Bind address to the socket
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(res);

    // Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued
	if (listen(sockfd, 5) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

    return sockfd;
}


int dns_upstream_connection(char *address, char *port, char *response, uint8_t *request, size_t request_len){
	char buffer[1024];
	// int s, upstream_sockfd;
	int read_len=0, n=0;
	// struct addrinfo hints, *servinfo, *rp;
	
	struct in_addr iaddr;

	inet_pton(AF_INET, address, &iaddr);

	struct sockaddr_in serv_addr;
	int connfd = 0;
	memset(&serv_addr, 0, sizeof serv_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(address);
	serv_addr.sin_port = htons((uint16_t)53);

	connfd = socket(AF_INET, SOCK_STREAM, 0);
	if (connfd < 0) printf("socket connection error");
	int y;
	y = connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	/* send data */
	int x=0;
	x = write(connfd, request, request_len+2);
	printf("wrote to server with length %d\n", x);

	FILE *fp1 = fopen("out1", "w");
	FILE *fp2 = fopen("out2", "w");
	x=fwrite(request, sizeof(char), request_len, fp1);
	x=fwrite(request, sizeof(char), request_len+2, fp2);
	fclose(fp1);
	fclose(fp2);

	printf("trying to recieve server data...\n");
	/* receive data */
	while ((n = read(connfd, buffer, 1023)) > 0){
		printf("received data (%d bytes): ", n);
		fwrite(buffer, 1023, 1, stdout);
		printf("$\n");
	}

	// memset(&hints, 0, sizeof hints);
	// hints.ai_family = AF_INET;
	// hints.ai_socktype = SOCK_STREAM;
	// s = getaddrinfo(address, port, &hints, &servinfo);
	// if (s != 0) {
	// 	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	// 	exit(EXIT_FAILURE);
	// }
	// printf("resolved address of host\n");
	// for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
	// 	upstream_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	// 	if (upstream_sockfd == -1)
	// 		continue;

	// 	if (connect(upstream_sockfd, rp->ai_addr, rp->ai_addrlen) != -1){
	// 		break; // success
	// 	}

	// 	close(upstream_sockfd);	/* close the failed socket */
	// }
	// if (rp == NULL) {
	// 	fprintf(stderr, "client: failed to connect\n");
	// 	exit(EXIT_FAILURE);
	// }
	// freeaddrinfo(servinfo);
	// printf("socket connected\n");

	// // Send message to server
	// n = write(upstream_sockfd, request, request_len+2);
	// if (n < 0) {
	// 	perror("socket");
	// 	exit(EXIT_FAILURE);
	// }

	// FILE *FD = fopen("out", "w");
	// printf("written to socket:\n\trequest_buffer = <<");
    // fwrite(request, request_len, 1, FD);
    // printf(">> with (written) length %d\n\n", n);
	// fclose(FD);

	// // Read message from server
	// while ((n = read(upstream_sockfd, buffer + read_len, 1024)) > 0){
	// 	printf(".");
	// 	read_len += n;
	// }
	// if (n < 0) {
	// 	perror("read");
	// 	exit(EXIT_FAILURE);
	// }
	// printf("readlen %d\tn %d\n", read_len, n);
	// printf("hhhhhhhhh?\n");
	// Null-terminate string
	// buffer[read_len] = '\0';
	// printf("%s\n", buffer);

	// close(upstream_sockfd);

	printf("finished?\n");
	fwrite(buffer, 1023, 1, stdout);
	printf("n is %d\n", n);
	FILE *fo = fopen("output", "w");
	fwrite(buffer, 1023, 1, fo);
	fclose(fo);
	return n;
}
