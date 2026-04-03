#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main()
{
    struct sockaddr client_addr, server_addr;
	char buf[14];
	socklen_t serverlen;

	int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (sockfd < 0)
	{
		perror("socket");
		exit(1);
	}

    client_addr.sa_family = AF_UNIX;

	if (bind(sockfd, &client_addr, sizeof(client_addr)) < 0)
	{
		perror("bind");
		exit(1);
	}

    sprintf(buf, "%d", getpid());
    server_addr.sa_family = AF_UNIX;
	strcpy(client_addr.sa_data, buf);
	strcpy(server_addr.sa_data, "mysocket.s");

	if (sendto(sockfd, buf, strlen(buf) + 1, 0, &server_addr, sizeof(server_addr)) < 0)
	{
		perror("sendto");
        exit(1);
	}

	printf("Client SEND message: %d\n", getpid());

	close(sockfd);
	unlink(buf);

	return EXIT_SUCCESS;
}