#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sodium.h>
#include "crypt.h"

int main(int argc, char *argv[]) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s, j;
	size_t len;
	ssize_t nread;
	char buf[BUF_SIZE];


	if(sodium_init() == -1) {
		fprintf(stderr, "sodium_init failed.\n");
		return 1;
	}

	if(argc < 4) {
		fprintf(stderr, "Usage: %s host port msg...\n", argv[0]);
		exit(EXIT_FAILURE);
	}



	/* Obtain address(es) matching host/port */

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	s = getaddrinfo(argv[1], argv[2], &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	 *               Try each address until we successfully connect(2).
	 *                             If socket(2) (or connect(2)) fails, we (close the socket
	 *                                           and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;                  /* Success */

		close(sfd);
	}

	if (rp == NULL) {               /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);           /* No longer needed */


	/* Send remaining command-line arguments as separate
	 *               datagrams, and read responses from server */

	char *massage = encrypt(argv[3]);
	char *massage1 = decrypt(massage);
	//printf("%s\n", massage1);
	len = get_crypt_len(massage);
	

	if (len > BUF_SIZE) {
		fprintf(stderr,
				"Ignoring long message in argument %d\n", j);
	}


	if (write(sfd, massage, len) != len) {
		fprintf(stderr, "partial/failed write\n");
		exit(EXIT_FAILURE);
	}
	free(massage);
	nread = read(sfd, buf, BUF_SIZE);
	if (nread == -1) {
		perror("read");
		exit(EXIT_FAILURE);
	}

	massage = decrypt(buf);
	printf("Received %zd bytes: %s\n", nread, massage);

	exit(EXIT_SUCCESS);
}


