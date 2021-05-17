#include "main.h"
#include "dns.h"

int main(int argc, char* argv[]) {
    
    int sockfd, newsockfd, n;
	char buffer[256];
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


		struct dns_message dns_request = parse_request(newsockfd);
		if (is_AAAA_record(dns_request) == false){
			set_rcode(&dns_request, RCODE_ERROR);
		} else {
			/* AAAA request is made, forward along to the upstream server */

			int s, upstream_sockfd;
			struct addrinfo hints, *servinfo, *rp;

			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			s = getaddrinfo(argv[1], argv[2], &hints, &servinfo);
			if (s != 0) {
				fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			printf("eeeeeeeee?\n");
			for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
				upstream_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
				if (upstream_sockfd == -1)
					continue;

				if (connect(upstream_sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
					break; // success

				close(upstream_sockfd);	/* close the failed socket */
			}
			printf("fffffffffff?\n");
			if (rp == NULL) {
				fprintf(stderr, "client: failed to connect\n");
				exit(EXIT_FAILURE);
			}
			freeaddrinfo(servinfo);

			// Send message to server
			n = write(upstream_sockfd, &dns_request, 68);
			if (n < 0) {
				perror("socket");
				exit(EXIT_FAILURE);
			}
			printf("gggggggggggggg?\n");

			// Read message from server
			n = read(upstream_sockfd, buffer, 255);
			if (n < 0) {
				perror("read");
				exit(EXIT_FAILURE);
			}
			printf("hhhhhhhhh?\n");
			// Null-terminate string
			buffer[n] = '\0';
			printf("%s\n", buffer);

			close(upstream_sockfd);

			printf("finished?\n");
		}



		// /* process the packet */
		// printf("Here is the message: %s\n", buffer);
		// n = write(newsockfd, "I got your message", 18);
		// if (n < 0) {
		// 	perror("write");
		// 	exit(EXIT_FAILURE);
		// }

		close(sockfd);
		close(newsockfd);
	} while (1);

    printf("done\n");
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
