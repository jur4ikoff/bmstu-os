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
    struct sockaddr server_addr, client_addr;
    socklen_t addrlen;
    char buff[BUFF_SIZE];

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        printf("errno: %d\n", errno);
        exit(1);
    }
    
    client_addr.sa_family = AF_UNIX;
    sprintf(client_addr.sa_data, "c-%d.s", getpid());

    if (bind(sockfd, &client_addr, sizeof(client_addr)) == -1) 
    {
        perror("bind");
        printf("errno %d\n", errno);
        exit(1);
    }
    
	server_addr.sa_family = AF_UNIX;
    strcpy(server_addr.sa_data, SOCK_NAME);

    sprintf(buff, "%d", getpid());
    printf("send: %s\n", buff);

    if (sendto(sockfd, buff, strlen(buff) + 1, 0, &server_addr, sizeof(server_addr)) == -1)
    {
        perror("sendto");
        printf("errno: %d\n", errno);
        exit(1);
    }

    if (recvfrom(sockfd, buff, sizeof(buff), 0, &server_addr, &addrlen) == -1) 
    {
        perror("recvfrom error");
        printf("errno %d\n", errno);
        exit(1);
    }
    printf("received: %s\n", buff);
    
    close(sockfd);
    if (unlink(client_addr.sa_data) == -1)
    {
    	perror("unlink");
    	printf("errno %d\n", errno);
    	exit(1);
    }
    return 0;
}
