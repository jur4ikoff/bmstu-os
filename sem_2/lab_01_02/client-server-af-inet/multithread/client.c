#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#define SERVER_PORT 1337
#define BUF_SIZE 2
#define RING_BUF_SIZE 26

int flag = 1;

void sig_handler(int sig_num)
{
	printf("catch sig %d\n", sig_num);
    flag = 0;
}

void request(int sockfd, char mode)
{
    int index;
    char buffer[BUF_SIZE];
    buffer[0] = mode;
    // printf("sending mode %c\n", buffer[0]);
    if (send(sockfd, buffer, sizeof(buffer), 0) == -1)
    {
        perror("send");
        exit(1);
    }

    // printf("sent, waiting for recv..\n");
    if (recv(sockfd, buffer, sizeof(buffer), 0) == -1)
    {
        perror("recieve");
        exit(1);
    }

    if (mode == 'c')
    {
        printf("[C] %d recv: %c\n", getpid(), buffer[1]);
    }
    else
    {
        printf("[P] %d recv: %d\n", getpid(), buffer[1]);
    }
    usleep(20000 + rand() % 150000);
}

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;

    if (argc != 3)
    {
        printf("usage: %s IP p/c", argv[0]);
        exit(1);
    }
    
    if (signal(SIGALRM, sig_handler) == SIG_ERR)
	{
		perror("signal");
		printf("errno: %d\n", errno);
		exit(1);
	}
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        printf("errno: %d\n", errno);
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) == -1)
    {
        perror("inet pton");
        printf("errno: %d\n", errno);
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("connect");
        printf("errno: %d\n", errno);
        exit(1);
    }
    else
        printf("Connected\n");

    while (flag) 
    {
        request(sockfd, argv[2][0]);
        // sleep(5);
    }

    close(sockfd);
    exit(0);
}