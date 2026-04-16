#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define SERV_PORT 9877
#define SIZE 64

int main(void)
{
    int sockfd;
    ssize_t n;
    struct sockaddr_in servaddr;
    char buff[SIZE];
    char send_char[3];
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(1);
    }
    for (long i = 0; i < 1000; ++i)
    {
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket error");
            exit(1);
        }
        if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
            perror("connect error");
            exit(1);
        }
        snprintf(send_char, sizeof(send_char), "%s", "p");
        if (i % 3 == 0)
        {
            snprintf(send_char, sizeof(send_char), "%s", "c");
        }
        printf("send %s\n", send_char);
        write(sockfd, send_char, 1);
        n = read(sockfd, buff, sizeof(buff));
        if (n > 0) {
            buff[n] = '\0';
            if (strncmp(buff, "buf full", 8) == 0)
                printf("buf full\n");
            else if (strncmp(buff, "buf empty", 9) == 0)
                printf("buf empty\n");
            else
                printf("receive: %s\n", buff);
        } else if (n == 0) {
            printf("server close connection\n");
        } else {
            perror("read\n");
        }

        close(sockfd);
    }
    return 0;
}
