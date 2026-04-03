#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main()
{
	struct sockaddr server_addr, client_addr;
	char buf[14];

    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (sockfd < 0)
	{
		perror("socket");
		return EXIT_FAILURE;
	}

	strcpy(server_addr.sa_data, "mysocket.s");
    server_addr.sa_family = AF_UNIX;
    socklen_t clientlen = sizeof(struct sockaddr); 

	if (bind(sockfd, &server_addr, sizeof(server_addr)) < 0)
	{
		perror("Bind error");
		return EXIT_FAILURE;
	}

	while(1)
	{
		int bytes = recvfrom(sockfd, buf, sizeof(buf), 0, &client_addr, &clientlen);

		if (bytes < 0)
		{
			perror("recvfrom");
			return EXIT_FAILURE;
		}

		printf("Server RECIEVED from client_addr: %s \n", buf);

		unlink(buf);
	}
    close(sockfd);

	return EXIT_SUCCESS;
}