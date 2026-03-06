#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SOCK_NAME "./server.s"
#define BUFF_SIZE 64

int main()
{
    int sockfd;
    struct sockaddr server_addr = { .sa_family = AF_UNIX };
    char buffer[BUFF_SIZE];

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        printf("errno: %d\n", errno);
        exit(1);
    }

    strcpy(server_addr.sa_data, SOCK_NAME);

    sprintf(buffer, "%d", getpid());
    printf("send: %s\n", buffer);

    if (sendto(sockfd, buffer, strlen(buffer), 0, &server_addr, sizeof(server_addr)) == -1)
    {
        perror("sendto");
        printf("errno: %d\n", errno);
        exit(1);
    }
    close(sockfd);

    return 0;
}
