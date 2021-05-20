#include "main.h"
#include "dns.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

size_t dns_upstream_connection(char *address, char *port, uint8_t **response, uint8_t *request, size_t request_len);

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
		uint8_t *request_buffer = NULL;
		size_t   request_len = parse_request(newsockfd, &dns_request, &request_buffer);
		uint8_t *response_buffer = NULL;
		size_t   response_len;

		if (is_AAAA_record(dns_request) == false){
			set_rcode_dns(&dns_request, RCODE_ERROR);
			set_rcode_char(request_buffer, RCODE_ERROR);

			/* return the modified request back to sender as reponse */
			response_buffer = request_buffer;
			response_len    = request_len;
		} else {
			/* AAAA request is made, forward along to the upstream server */
			response_len = dns_upstream_connection(argv[1], argv[2], &response_buffer, request_buffer, request_len);
		}

		/* return the packet back to the client over that original connection */
		n = write(newsockfd, response_buffer, response_len);
		printf("sent %d bytes\n", n);
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


size_t dns_upstream_connection(char *address, char *port, uint8_t **response, uint8_t *request, size_t request_len){
	// char buffer[256];
	// int s, upstream_sockfd;
	// int read_len=0, n;
	// struct addrinfo hints, *servinfo, *rp;

	// memset(&hints, 0, sizeof hints);
	// hints.ai_family = AF_INET;
	// hints.ai_socktype = SOCK_STREAM;
	// s = getaddrinfo(address, port, &hints, &servinfo);
	// if (s != 0) {
	// 	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	// 	exit(EXIT_FAILURE);
	// }
	// // printf("eeeeeeeee?\n" );
	// for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
	// 	upstream_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	// 	if (upstream_sockfd == -1)
	// 		continue;

	// 	if (connect(upstream_sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
	// 		break; // success

	// 	close(upstream_sockfd);	/* close the failed socket */
	// }
	// // printf("fffffffffff?\n");
	// if (rp == NULL) {
	// 	fprintf(stderr, "client: failed to connect\n");
	// 	exit(EXIT_FAILURE);

	char buffer[1024];
	// int s, upstream_sockfd;
	int read_len=0/*, n=0*/;
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
	if (connfd < 0)
		printf("socket connection error");
	int y;
	y = connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (y != 0)
		perror("connect");

	/* send data */
	int writelen=0;
	writelen = write(connfd, request, request_len);
	if (writelen != request_len) {
		perror("write");
	}

	/* receive data */
	read_len = read(connfd, buffer+read_len, 1023);
#if DNS_VERBOSE
	printf("received data (%d bytes) from upstream\n", read_len);
#endif

	*response = calloc(read_len, sizeof(*request));
	for (int i = 0; i<read_len;i++){
		(*response)[i] = buffer[i];
	}

	close(connfd);
	return (size_t) read_len;
}
