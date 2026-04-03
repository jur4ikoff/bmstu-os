#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERV_PORT 9878

void produce(int sockfd) {
    char buf[10];
    buf[0] = 'p';
    if (send(sockfd, &buf, sizeof(buf), 0) == -1) {
        perror("send");
        exit(1);
    }
    if (recv(sockfd, &buf, sizeof(buf), 0) == -1) {
        perror("recieve");
        exit(1);
    }
    if (buf[0] == 'o')
        printf("Success produce\n");
    else
        printf("Error produce\n");
}

void consume(int sockfd) {
    char buf[1];
    buf[0] = 'c';
    if (send(sockfd, &buf, sizeof(buf), 0) == -1) {
        perror("send");
        exit(1);
    }
    if (recv(sockfd, &buf, sizeof(buf), 0) == -1) {
        perror("recieve");
        exit(1);
    }
    if (buf[0] <= 'z' && buf[0] >= 'a')
        printf("Success consume: %c\n", buf[0]);
    else
        printf("Error consume\n");
}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr = { 0 };


    if (argc != 2) {
        perror("usage: client <IPaddress>");
        exit(1);
    }
    for (int i = 0; i < 500; i++) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            perror("socket");
            exit(1);
        }
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(SERV_PORT);
        if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) == -1) {
            perror("inet_pton");
            exit(1);
        }
        if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
            perror("connect");
            exit(1);
        }
        printf("Connected\n");
        if (i % 2 == 1)
            consume(sockfd);
        else
            produce(sockfd);
        close(sockfd);
        //usleep(rand() % 5000);
    }
    exit(0);
}
