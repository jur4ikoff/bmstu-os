#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define SOCK_NAME "./server.s"
#define BUFF_SIZE 64

int fl = 1;
void sig_handler(int signum) 
{
    printf("catch sig %d\n", signum);
    fl = 0;
}
int main() 
{
	int sockfd; 
    struct sockaddr server_addr, client_addr;
    uint addrlen;
    char buff[BUFF_SIZE];

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
    	perror("socket");
 	    printf("errno %d\n", errno);
		exit(1);
    }
	if (signal(SIGINT, sig_handler) == SIG_ERR)
	{
	    perror("signal");
        printf("errno %d\n", errno);
	    exit(1);
	}
    server_addr.sa_family = AF_UNIX;
    strcpy(server_addr.sa_data, SOCK_NAME);
    if (bind(sockfd, &server_addr, sizeof(server_addr)) == -1) 
    {
        perror("bind");
        printf("errno %d\n", errno);
        exit(1);
    }
    while (fl) 
    {
        if (recvfrom(sockfd, buff, BUFF_SIZE, 0, &client_addr, &addrlen) == -1) 
        {
            perror("recvfrom error");
            printf("errno %d\n", errno);
            exit(1);
        }
        printf("received: %s\n", buff);
    }

    if (unlink(SOCK_NAME) == -1)
    {
    	perror("unlink");
    	printf("errno %d\n", errno);
    	exit(1);
    }
    exit(0);
}
